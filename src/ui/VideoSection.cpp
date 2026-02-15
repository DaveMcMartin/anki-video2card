#include "ui/VideoSection.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <mpv/client.h>
#include <mpv/render.h>

#include "IconsFontAwesome6.h"
#include "config/ConfigManager.h"
#include "core/Logger.h"
#include "language/ILanguage.h"
#include "utils/LastVideoPath.h"
#include "utils/VideoState.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cmath>
#include <iostream>
#include <webp/encode.h>

namespace Video2Card::UI
{

  static void* get_proc_address_mpv(void* fn_ctx, const char* name)
  {
    return nullptr; // For SW render we don't need GL proc address
  }

  VideoSection::VideoSection(SDL_Renderer* renderer,
                             Config::ConfigManager* configManager,
                             std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                             Language::ILanguage** activeLanguage)
      : m_Renderer(renderer)
      , m_ConfigManager(configManager)
      , m_Languages(languages)
      , m_ActiveLanguage(activeLanguage)
  {
    InitializeMPV();
  }

  VideoSection::~VideoSection()
  {
    DestroyMPV();
    if (m_VideoTexture) {
      SDL_DestroyTexture(m_VideoTexture);
    }
  }

  void VideoSection::InitializeMPV()
  {
    m_mpv = mpv_create();
    if (!m_mpv) {
      AF_ERROR("Failed to create mpv context");
      return;
    }

    mpv_set_option_string(m_mpv, "config", "no");
    mpv_set_option_string(m_mpv, "terminal", "yes");
    mpv_set_option_string(m_mpv, "msg-level", "all=warn");
    mpv_set_option_string(m_mpv, "vd-lavc-threads", "4");
    mpv_set_option_string(m_mpv, "vo", "libmpv");

    if (mpv_initialize(m_mpv) < 0) {
      AF_ERROR("Failed to initialize mpv");
      return;
    }

    mpv_render_param params[] = {{MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_SW)},
                                 {MPV_RENDER_PARAM_INVALID, nullptr}};

    if (mpv_render_context_create(&m_mpv_render, m_mpv, params) < 0) {
      std::cerr << "Failed to create mpv render context" << std::endl;
    }

    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "width", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpv, 0, "height", MPV_FORMAT_INT64);
  }

  void VideoSection::DestroyMPV()
  {
    if (m_mpv_render) {
      mpv_render_context_free(m_mpv_render);
      m_mpv_render = nullptr;
    }
    if (m_mpv) {
      mpv_terminate_destroy(m_mpv);
      m_mpv = nullptr;
    }
  }

  void VideoSection::LoadVideoFromFile(const std::string& path)
  {
    if (!m_mpv)
      return;

    AF_INFO("Loading video file: {}", path);
    m_CurrentVideoPath = path;
    m_FileLoadedSuccessfully = false;
    m_PendingSeekPosition = -1.0;

    uint64_t savedPosition = Utils::VideoState::LoadPlaybackPosition(path);
    if (savedPosition > 0) {
      m_PendingSeekPosition = savedPosition / 1000.0;
      AF_DEBUG("Deferred seek position set to: {} seconds", m_PendingSeekPosition);
    }

    // Save the last loaded video path for restoration on next startup
    Utils::LastVideoPath::Save(path);

    const char* cmd[] = {"loadfile", path.c_str(), nullptr};
    int res = mpv_command_async(m_mpv, 0, cmd);
    if (res < 0) {
      AF_ERROR("Failed to send loadfile command: {}", mpv_error_string(res));
    }

    m_IsPlaying = true;
    int pause = 0;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &pause);
    m_LastSaveTime = 0.0;
  }

  void VideoSection::ClearVideo()
  {
    if (!m_CurrentVideoPath.empty() && m_CurrentTime > 0) {
      uint64_t positionMs = static_cast<uint64_t>(m_CurrentTime * 1000);
      Utils::VideoState::SavePlaybackPosition(m_CurrentVideoPath, positionMs);
      AF_DEBUG("Saved playback position: {} ms", positionMs);
    }
    // Clear the last video path when user explicitly clears the video
    Utils::LastVideoPath::Clear();
    m_ShouldClearVideo = true;
  }

  void VideoSection::Update()
  {
    if (m_ShouldClearVideo) {
      if (m_mpv) {
        const char* cmd[] = {"stop", nullptr};
        mpv_command_async(m_mpv, 0, cmd);
      }
      m_CurrentVideoPath.clear();
      m_IsPlaying = false;
      m_Duration = 0.0;
      m_CurrentTime = 0.0;

      if (m_VideoTexture) {
        SDL_DestroyTexture(m_VideoTexture);
        m_VideoTexture = nullptr;
      }
      m_VideoWidth = 0;
      m_VideoHeight = 0;
      m_FrameBuffer.clear();
      m_ShouldClearVideo = false;
    }

    HandleMPVEvents();
    UpdateVideoTexture();

    // Periodically save playback position to avoid excessive disk writes
    if (!m_CurrentVideoPath.empty() && m_CurrentTime > 0) {
      if (m_CurrentTime - m_LastSaveTime >= SAVE_INTERVAL) {
        uint64_t positionMs = static_cast<uint64_t>(m_CurrentTime * 1000);
        Utils::VideoState::SavePlaybackPosition(m_CurrentVideoPath, positionMs);
        m_LastSaveTime = m_CurrentTime;
      }
    }
  }

  void VideoSection::HandleMPVEvents()
  {
    if (!m_mpv)
      return;

    while (true) {
      mpv_event* event = mpv_wait_event(m_mpv, 0);
      if (event->event_id == MPV_EVENT_NONE)
        break;

      if (event->event_id == MPV_EVENT_FILE_LOADED) {
        m_FileLoadedSuccessfully = true;
        AF_DEBUG("File loaded event received");
        if (m_PendingSeekPosition >= 0.0) {
          AF_INFO("Performing deferred seek to {} seconds", m_PendingSeekPosition);
          mpv_set_property_async(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE, &m_PendingSeekPosition);
          m_PendingSeekPosition = -1.0;
        }
      } else if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
        mpv_event_property* prop = (mpv_event_property*) event->data;
        if (prop->data == nullptr)
          continue;

        std::string name = prop->name;
        if (name == "time-pos" && prop->format == MPV_FORMAT_DOUBLE) {
          m_CurrentTime = *(double*) prop->data;
        } else if (name == "duration" && prop->format == MPV_FORMAT_DOUBLE) {
          m_Duration = *(double*) prop->data;
        } else if (name == "pause" && prop->format == MPV_FORMAT_FLAG) {
          m_IsPlaying = !(*(int*) prop->data);
        } else if (name == "width" && prop->format == MPV_FORMAT_INT64) {
          m_VideoWidth = (int) *(int64_t*) prop->data;
          AF_INFO("Video width changed: {}", m_VideoWidth);
        } else if (name == "height" && prop->format == MPV_FORMAT_INT64) {
          m_VideoHeight = (int) *(int64_t*) prop->data;
          AF_INFO("Video height changed: {}", m_VideoHeight);
        }
      } else if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
        mpv_event_log_message* msg = (mpv_event_log_message*) event->data;
        AF_DEBUG("MPV: [{}] {}", msg->prefix, msg->text);
      } else if (event->event_id == MPV_EVENT_START_FILE) {
        AF_INFO("MPV: Start file");
      } else if (event->event_id == MPV_EVENT_END_FILE) {
        AF_INFO("MPV: End file");
      }
    }
  }

  void VideoSection::UpdateVideoTexture()
  {
    if (!m_mpv_render)
      return;

    if (m_VideoWidth <= 0 || m_VideoHeight <= 0)
      return;

    // Check if we need to resize texture
    bool textureNeedsUpdate = false;
    if (!m_VideoTexture) {
      textureNeedsUpdate = true;
    } else {
      float w = 0, h = 0;
      SDL_GetTextureSize(m_VideoTexture, &w, &h);
      if ((int) w != m_VideoWidth || (int) h != m_VideoHeight) {
        textureNeedsUpdate = true;
      }
    }

    if (textureNeedsUpdate) {
      AF_INFO("Recreating video texture: {}x{}", m_VideoWidth, m_VideoHeight);
      if (m_VideoTexture)
        SDL_DestroyTexture(m_VideoTexture);
      m_VideoTexture = SDL_CreateTexture(
          m_Renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, m_VideoWidth, m_VideoHeight);

      if (!m_VideoTexture) {
        AF_ERROR("Failed to create video texture: {}", SDL_GetError());
        return;
      }
      m_FrameBuffer.resize(m_VideoWidth * m_VideoHeight * 4);
    }

    // Render frame
    int stride = m_VideoWidth * 4;

    // We actually need to provide the buffer to SW render
    // But mpv_render_context_render for SW expects an array of pointers to planes
    // and strides.

    uint8_t* data = m_FrameBuffer.data();
    mpv_render_frame_info frame_info = {0};
    frame_info.flags = 0;

    // For SW render, we use a different approach or just use the raw pointer
    // The MPV API for SW render is a bit specific.
    // We need to pass the buffer in params.

    int size[2] = {m_VideoWidth, m_VideoHeight};
    const char* format = "rgba";

    mpv_render_param sw_params[] = {{MPV_RENDER_PARAM_SW_SIZE, size},
                                    {MPV_RENDER_PARAM_SW_FORMAT, (void*) format},
                                    {MPV_RENDER_PARAM_SW_STRIDE, &stride},
                                    {MPV_RENDER_PARAM_SW_POINTER, data},
                                    {MPV_RENDER_PARAM_INVALID, nullptr}};

    // Only update if necessary? mpv_render_context_update tells us.
    // But for simplicity in this loop, we can try to render.
    // mpv_render_context_update returns flags indicating if redraw is needed.

    uint64_t flags = mpv_render_context_update(m_mpv_render);
    if (flags & MPV_RENDER_UPDATE_FRAME) {
      int res = mpv_render_context_render(m_mpv_render, sw_params);
      if (res >= 0) {
        SDL_UpdateTexture(m_VideoTexture, nullptr, m_FrameBuffer.data(), stride);
      }
    }
  }

  void VideoSection::Render()
  {
    ImGui::Begin("Video Player",
                 nullptr,
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

    if (m_Languages && m_ActiveLanguage && *m_ActiveLanguage) {
      ImGui::SetNextItemWidth(150);
      if (ImGui::BeginCombo("##Language", (*m_ActiveLanguage)->GetName().c_str())) {
        for (auto& lang : *m_Languages) {
          bool isSelected = (lang.get() == *m_ActiveLanguage);
          if (ImGui::Selectable(lang->GetName().c_str(), isSelected)) {
            *m_ActiveLanguage = lang.get();
            if (m_ConfigManager) {
              m_ConfigManager->GetConfig().SelectedLanguage = lang->GetIdentifier();
              m_ConfigManager->Save();
            }
          }
          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::SameLine();
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Language");
    }

    // Calculate available space for video area (leaving space for controls at bottom)
    float footerHeight = 110.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 videoAreaSize = ImVec2(avail.x, std::max(50.0f, avail.y - footerHeight));

    // Draw border/background for video area
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::BeginChild("VideoArea", videoAreaSize, true);

    if (m_VideoTexture) {
      // Maintain aspect ratio
      float availWidth = ImGui::GetContentRegionAvail().x;
      float availHeight = ImGui::GetContentRegionAvail().y;

      float scale = std::min(availWidth / m_VideoWidth, availHeight / m_VideoHeight);
      float displayW = m_VideoWidth * scale;
      float displayH = m_VideoHeight * scale;

      // Center
      float cursorX = (availWidth - displayW) * 0.5f;
      float cursorY = (availHeight - displayH) * 0.5f;

      ImGui::SetCursorPos(ImVec2(cursorX, cursorY));
      ImGui::Image((ImTextureID) (intptr_t) m_VideoTexture, ImVec2(displayW, displayH));
    } else {
      // Center text
      const char* text = "Drop Video Here";
      ImVec2 textSize = ImGui::CalcTextSize(text);
      ImVec2 windowSize = ImGui::GetWindowSize();
      ImGui::SetCursorPos(ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f));
      ImGui::Text("%s", text);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (!m_CurrentVideoPath.empty()) {
      size_t lastSlash = m_CurrentVideoPath.find_last_of("/\\");
      std::string filename =
          (lastSlash != std::string::npos) ? m_CurrentVideoPath.substr(lastSlash + 1) : m_CurrentVideoPath;
      ImGui::Text("%s", filename.c_str());
    }

    DrawControls();

    // Shortcuts
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
      if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        TogglePlayback();
      }
      if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        Seek(5.0);
      }
      if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        Seek(-5.0);
      }
      if (ImGui::IsKeyPressed(ImGuiKey_M)) {
        if (m_OnExtractCallback)
          m_OnExtractCallback();
      }
    }

    ImGui::End();
  }

  void VideoSection::DrawControls()
  {
    ImGui::Spacing();

    // Progress Bar
    float progress = (m_Duration > 0) ? (float) (m_CurrentTime / m_Duration) : 0.0f;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
    ImGui::PushItemWidth(-1);
    if (ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "")) {
      SeekAbsolute(progress * m_Duration);
    }
    ImGui::PopItemWidth();
    ImGui::PopStyleColor(2);

    ImGui::Spacing();

    // Controls Row
    std::string timeStr = std::format("{:02d}:{:02d} / {:02d}:{:02d}",
                                      (int) m_CurrentTime / 60,
                                      (int) m_CurrentTime % 60,
                                      (int) m_Duration / 60,
                                      (int) m_Duration % 60);
    ImGui::Text("%s", timeStr.c_str());

    ImGui::SameLine();

    float buttonWidth = 40.0f;

    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

    if (ImGui::Button(m_IsPlaying ? ICON_FA_PAUSE : ICON_FA_PLAY, ImVec2(buttonWidth, 0))) {
      TogglePlayback();
    }

    ImGui::SameLine();
    if (ImGui::Button("-5s", ImVec2(buttonWidth, 0)))
      Seek(-5.0);
    ImGui::SameLine();
    if (ImGui::Button("+5s", ImVec2(buttonWidth, 0)))
      Seek(5.0);

    ImGui::PopItemFlag();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    float volume = (float) m_Volume;
    if (ImGui::SliderFloat("##vol", &volume, 0.0f, 100.0f, "Vol %.0f")) {
      m_Volume = volume;
      double vol = m_Volume;
      mpv_set_property_async(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE, &vol);
    }

    ImGui::SameLine();

    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (ImGui::Button(ICON_FA_TRASH, ImVec2(buttonWidth, 0))) {
      ClearVideo();
    }
    ImGui::PopItemFlag();

    ImGui::SameLine();

    // Push Extract to the right
    float availX = ImGui::GetContentRegionAvail().x;
    float extractWidth = 120.0f;
    if (availX > extractWidth) {
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availX - extractWidth);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
    if (ImGui::Button(ICON_FA_CROP " Extract", ImVec2(extractWidth, 0))) {
      if (m_OnExtractCallback)
        m_OnExtractCallback();
    }
    ImGui::PopStyleColor(3);
  }

  void VideoSection::TogglePlayback()
  {
    if (!m_mpv)
      return;
    int flag = !m_IsPlaying ? 0 : 1;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
  }

  void VideoSection::Seek(double seconds)
  {
    if (!m_mpv)
      return;
    std::string secondsStr = std::to_string(seconds);
    const char* cmd[] = {"seek", secondsStr.c_str(), "relative", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
  }

  void VideoSection::SeekAbsolute(double timestamp)
  {
    if (!m_mpv)
      return;
    std::string timestampStr = std::to_string(timestamp);
    const char* cmd[] = {"seek", timestampStr.c_str(), "absolute", nullptr};
    mpv_command_async(m_mpv, 0, cmd);
  }

  std::vector<unsigned char> VideoSection::GetCurrentFrameImage()
  {
    if (m_FrameBuffer.empty() || m_VideoWidth <= 0 || m_VideoHeight <= 0) {
      return {};
    }

    // Calculate new dimensions (max 320x320)
    int newWidth = m_VideoWidth;
    int newHeight = m_VideoHeight;

    if (newWidth > 320 || newHeight > 320) {
      float scale = std::min(320.0f / newWidth, 320.0f / newHeight);
      newWidth = (int) (newWidth * scale);
      newHeight = (int) (newHeight * scale);
    }

    // Scale using libswscale
    struct SwsContext* sws_ctx = sws_getContext(m_VideoWidth,
                                                m_VideoHeight,
                                                AV_PIX_FMT_RGBA,
                                                newWidth,
                                                newHeight,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                nullptr,
                                                nullptr,
                                                nullptr);

    if (!sws_ctx) {
      AF_ERROR("Failed to create sws context for image resizing");
      return {};
    }

    std::vector<uint8_t> scaledBuffer(newWidth * newHeight * 4);
    uint8_t* srcSlice[] = {m_FrameBuffer.data()};
    int srcStride[] = {m_VideoWidth * 4};
    uint8_t* dstSlice[] = {scaledBuffer.data()};
    int dstStride[] = {newWidth * 4};

    sws_scale(sws_ctx, srcSlice, srcStride, 0, m_VideoHeight, dstSlice, dstStride);
    sws_freeContext(sws_ctx);

    // Encode to WebP
    uint8_t* output;
    size_t size = WebPEncodeRGBA(scaledBuffer.data(), newWidth, newHeight, newWidth * 4, 90.0f, &output);

    if (size > 0) {
      std::vector<unsigned char> result(output, output + size);
      WebPFree(output);
      return result;
    }

    return {};
  }

  SubtitleData VideoSection::GetCurrentSubtitle()
  {
    SubtitleData data;
    if (!m_mpv)
      return data;

    char* text = nullptr;
    mpv_get_property(m_mpv, "sub-text", MPV_FORMAT_STRING, &text);
    if (text) {
      data.text = text;
      mpv_free(text);
    }

    mpv_get_property(m_mpv, "sub-start", MPV_FORMAT_DOUBLE, &data.start);
    mpv_get_property(m_mpv, "sub-end", MPV_FORMAT_DOUBLE, &data.end);

    // If sub-start/end are not valid (e.g. no sub), they might be 0 or NaN.
    // MPV usually returns relative time for sub-start/end? No, usually absolute timestamp?
    // Actually sub-start/end properties are often not reliable for "current sub" timing in all mpv versions.
    // But let's assume they work or we use current time.

    // If we can't get precise timing, default to a window around current time
    if (data.start == 0.0 && data.end == 0.0 && !data.text.empty()) {
      data.start = m_CurrentTime;
      data.end = m_CurrentTime + 2.0; // Default duration
    }

    return data;
  }

  double VideoSection::GetCurrentTimestamp()
  {
    return m_CurrentTime;
  }

  // Audio Extraction using FFmpeg
  struct IOContext
  {
    std::vector<uint8_t> buffer;
    int pos = 0;
  };

  static int write_packet(void* opaque, const uint8_t* buf, int buf_size)
  {
    IOContext* ctx = (IOContext*) opaque;
    ctx->buffer.insert(ctx->buffer.end(), buf, buf + buf_size);
    ctx->pos += buf_size;
    return buf_size;
  }

  std::vector<unsigned char> VideoSection::GetAudioClip(double start, double end)
  {
    if (m_CurrentVideoPath.empty())
      return {};

    AF_INFO("Extracting audio from {} to {}", start, end);

    av_log_set_level(AV_LOG_QUIET);

    AVFormatContext* inputFormatContext = nullptr;
    if (avformat_open_input(&inputFormatContext, m_CurrentVideoPath.c_str(), nullptr, nullptr) < 0) {
      AF_ERROR("Failed to open input file for audio extraction");
      return {};
    }

    if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
      AF_ERROR("Failed to find stream info");
      avformat_close_input(&inputFormatContext);
      return {};
    }

    int audioStreamIndex = -1;
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++) {
      if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        audioStreamIndex = i;
        break;
      }
    }

    if (audioStreamIndex == -1) {
      AF_ERROR("No audio stream found");
      avformat_close_input(&inputFormatContext);
      return {};
    }

    // Transcode to OGG/Vorbis

    // --- Setup Input Decoder ---
    AVCodecParameters* inCodecPar = inputFormatContext->streams[audioStreamIndex]->codecpar;
    const AVCodec* inCodec = avcodec_find_decoder(inCodecPar->codec_id);
    AVCodecContext* inCodecCtx = avcodec_alloc_context3(inCodec);
    avcodec_parameters_to_context(inCodecCtx, inCodecPar);
    if (avcodec_open2(inCodecCtx, inCodec, nullptr) < 0) {
      AF_ERROR("Failed to open input codec");
      avcodec_free_context(&inCodecCtx);
      avformat_close_input(&inputFormatContext);
      return {};
    }

    // --- Setup Output (OGG/Vorbis) ---
    IOContext ioCtx;
    const int avio_buffer_size = 4096;
    unsigned char* avio_buffer = (unsigned char*) av_malloc(avio_buffer_size);
    AVIOContext* avioContext =
        avio_alloc_context(avio_buffer, avio_buffer_size, 1, &ioCtx, nullptr, write_packet, nullptr);

    AVFormatContext* outputFormatContext = nullptr;
    avformat_alloc_output_context2(&outputFormatContext, nullptr, "ogg", nullptr);
    outputFormatContext->pb = avioContext;

    const AVCodec* outCodec = avcodec_find_encoder(AV_CODEC_ID_VORBIS);
    AVStream* outStream = avformat_new_stream(outputFormatContext, outCodec);
    AVCodecContext* outCodecCtx = avcodec_alloc_context3(outCodec);

    // Configure output audio parameters
    outCodecCtx->sample_rate = 44100;
    av_channel_layout_default(&outCodecCtx->ch_layout, 2);
    outCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    outCodecCtx->time_base = (AVRational) {1, outCodecCtx->sample_rate};
    outCodecCtx->bit_rate = 128000;

    if (avcodec_open2(outCodecCtx, outCodec, nullptr) < 0) {
      AF_ERROR("Failed to open output codec");
      avcodec_free_context(&inCodecCtx);
      avcodec_free_context(&outCodecCtx);
      avformat_close_input(&inputFormatContext);
      avformat_free_context(outputFormatContext);
      av_free(avioContext->buffer);
      av_free(avioContext);
      return {};
    }
    avcodec_parameters_from_context(outStream->codecpar, outCodecCtx);

    if (avformat_write_header(outputFormatContext, nullptr) < 0) {
      AF_ERROR("Failed to write header");
      avcodec_free_context(&inCodecCtx);
      avcodec_free_context(&outCodecCtx);
      avformat_close_input(&inputFormatContext);
      avformat_free_context(outputFormatContext);
      av_free(avioContext->buffer);
      av_free(avioContext);
      return {};
    }

    // --- Setup FIFO ---
    AVAudioFifo* fifo = av_audio_fifo_alloc(outCodecCtx->sample_fmt, outCodecCtx->ch_layout.nb_channels, 1);

    // --- Setup Resampler ---
    SwrContext* swrCtx = nullptr;
    // Defer initialization to first frame

    // --- Processing Loop ---

    // Seek
    int64_t seekTarget = (int64_t) (start * AV_TIME_BASE);
    av_seek_frame(inputFormatContext, -1, seekTarget, AVSEEK_FLAG_BACKWARD);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* resampledFrame = av_frame_alloc(); // Frame to hold resampler output
    AVFrame* fifoFrame = av_frame_alloc();      // Frame to hold fixed-size chunks for encoder

    // Prepare fifoFrame (fixed size for encoder)
    fifoFrame->nb_samples = outCodecCtx->frame_size;
    fifoFrame->format = outCodecCtx->sample_fmt;
    av_channel_layout_copy(&fifoFrame->ch_layout, &outCodecCtx->ch_layout);
    fifoFrame->sample_rate = outCodecCtx->sample_rate;
    av_frame_get_buffer(fifoFrame, 0);

    // Prepare resampledFrame (variable size)
    resampledFrame->format = outCodecCtx->sample_fmt;
    av_channel_layout_copy(&resampledFrame->ch_layout, &outCodecCtx->ch_layout);
    resampledFrame->sample_rate = outCodecCtx->sample_rate;

    bool finished = false;
    int framesProcessed = 0;
    int64_t pts = 0; // Track PTS manually for output

    while (av_read_frame(inputFormatContext, packet) >= 0 && !finished) {
      if (packet->stream_index == audioStreamIndex) {
        if (avcodec_send_packet(inCodecCtx, packet) == 0) {
          while (avcodec_receive_frame(inCodecCtx, frame) == 0) {
            double currentTimestamp = frame->pts * av_q2d(inputFormatContext->streams[audioStreamIndex]->time_base);

            if (!swrCtx) {
              swr_alloc_set_opts2(&swrCtx,
                                  &outCodecCtx->ch_layout,
                                  outCodecCtx->sample_fmt,
                                  outCodecCtx->sample_rate,
                                  &frame->ch_layout,
                                  (enum AVSampleFormat) frame->format,
                                  frame->sample_rate,
                                  0,
                                  nullptr);
              swr_init(swrCtx);
            }

            if (currentTimestamp > end) {
              AF_INFO("Reached end time: {} > {}", currentTimestamp, end);
              finished = true;
              break;
            }

            if (currentTimestamp + (double) frame->nb_samples / frame->sample_rate >= start) {
              framesProcessed++;

              // Calculate output samples
              int out_samples = av_rescale_rnd(swr_get_delay(swrCtx, frame->sample_rate) + frame->nb_samples,
                                               outCodecCtx->sample_rate,
                                               frame->sample_rate,
                                               AV_ROUND_UP);

              if (resampledFrame->nb_samples < out_samples) {
                av_frame_unref(resampledFrame);
                resampledFrame->nb_samples = out_samples;
                resampledFrame->format = outCodecCtx->sample_fmt;
                av_channel_layout_copy(&resampledFrame->ch_layout, &outCodecCtx->ch_layout);
                resampledFrame->sample_rate = outCodecCtx->sample_rate;
                av_frame_get_buffer(resampledFrame, 0);
              }

              // Resample
              int ret = swr_convert(
                  swrCtx, resampledFrame->data, out_samples, (const uint8_t**) frame->data, frame->nb_samples);
              if (ret > 0) {
                // Add to FIFO
                if (av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + ret) < 0) {
                  AF_ERROR("Failed to realloc FIFO");
                  break;
                }
                av_audio_fifo_write(fifo, (void**) resampledFrame->data, ret);

                // Process FIFO
                while (av_audio_fifo_size(fifo) >= outCodecCtx->frame_size) {
                  // Read from FIFO
                  if (av_audio_fifo_read(fifo, (void**) fifoFrame->data, outCodecCtx->frame_size) <
                      outCodecCtx->frame_size)
                  {
                    break;
                  }

                  fifoFrame->pts = pts;
                  pts += fifoFrame->nb_samples;

                  if (avcodec_send_frame(outCodecCtx, fifoFrame) == 0) {
                    AVPacket* outPacket = av_packet_alloc();
                    while (avcodec_receive_packet(outCodecCtx, outPacket) == 0) {
                      outPacket->stream_index = 0;
                      av_packet_rescale_ts(outPacket, outCodecCtx->time_base, outStream->time_base);
                      av_interleaved_write_frame(outputFormatContext, outPacket);
                      AF_DEBUG("Wrote audio packet, size: {}", outPacket->size);
                      av_packet_unref(outPacket);
                    }
                    av_packet_free(&outPacket);
                  }
                }
              }
            }
          }
        }
      }
      av_packet_unref(packet);
    }

    AF_INFO("Processed {} audio frames from input", framesProcessed);

    // Flush remaining samples in FIFO
    int remaining = av_audio_fifo_size(fifo);
    if (remaining > 0) {
      AF_INFO("Flushing remaining {} samples from FIFO", remaining);
      if (av_audio_fifo_read(fifo, (void**) fifoFrame->data, remaining) == remaining) {
        fifoFrame->nb_samples = remaining;
        fifoFrame->pts = pts;

        if (avcodec_send_frame(outCodecCtx, fifoFrame) == 0) {
          AVPacket* outPacket = av_packet_alloc();
          while (avcodec_receive_packet(outCodecCtx, outPacket) == 0) {
            outPacket->stream_index = 0;
            av_packet_rescale_ts(outPacket, outCodecCtx->time_base, outStream->time_base);
            av_interleaved_write_frame(outputFormatContext, outPacket);
            av_packet_unref(outPacket);
          }
          av_packet_free(&outPacket);
        }
      }
    }

    // Flush encoder
    AF_INFO("Flushing audio encoder...");
    avcodec_send_frame(outCodecCtx, nullptr);
    AVPacket* outPacket = av_packet_alloc();
    while (avcodec_receive_packet(outCodecCtx, outPacket) == 0) {
      outPacket->stream_index = 0;
      av_packet_rescale_ts(outPacket, outCodecCtx->time_base, outStream->time_base);
      av_interleaved_write_frame(outputFormatContext, outPacket);
      AF_DEBUG("Wrote flushed audio packet, size: {}", outPacket->size);
      av_packet_unref(outPacket);
    }
    av_packet_free(&outPacket);

    av_write_trailer(outputFormatContext);

    // Cleanup
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_frame_free(&resampledFrame);
    av_frame_free(&fifoFrame);
    av_audio_fifo_free(fifo);
    swr_free(&swrCtx);
    avcodec_free_context(&inCodecCtx);
    avcodec_free_context(&outCodecCtx);
    avformat_close_input(&inputFormatContext);
    avformat_free_context(outputFormatContext);
    av_free(avioContext->buffer);
    av_free(avioContext);

    AF_INFO("Audio extraction finished, size: {}", ioCtx.buffer.size());
    return ioCtx.buffer;
  }

} // namespace Video2Card::UI

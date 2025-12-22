#include "ui/fields/CardField.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <filesystem>
#include <fstream>

#include "IconsFontAwesome6.h"
#include "core/FieldTypes.h"
#include "core/Logger.h"
#include "core/sdl/SDLWrappers.h"
#include "stb_image.h"
#include <webp/decode.h>

namespace Video2Card
{
  namespace UI
  {

    CardField::CardField(const std::string& name)
        : m_Name(name)
        , m_AudioPlayer(std::make_unique<Audio::AudioPlayer>())
    {}

    CardField::~CardField()
    {
      if (m_ImageTexture) {
        SDL_DestroyTexture(m_ImageTexture);
      }
    }

    void CardField::Render(SDL_Renderer* renderer)
    {
      ImGui::PushID(m_Name.c_str());

      ImGui::Text("%s", m_Name.c_str());

      ImGui::Checkbox("Enable Auto-Fill", &m_IsToolEnabled);

      ImGui::SameLine();

      ImGui::BeginDisabled(m_IsToolEnabled);
      RenderTypeSelector();
      ImGui::EndDisabled();

      if (m_IsToolEnabled) {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##ToolSelect", Video2Card::Core::FieldToolNames[m_SelectedToolIndex].data())) {
          for (int i = 0; i < static_cast<int>(Video2Card::Core::FieldToolNames.size()); i++) {
            const bool isSelected = (m_SelectedToolIndex == i);
            if (ImGui::Selectable(Video2Card::Core::FieldToolNames[i].data(), isSelected)) {
              m_SelectedToolIndex = i;
            }

            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }
      }

      switch (m_Type) {
        case CardFieldType::Text:
          RenderTextContent();
          break;
        case CardFieldType::Audio:
          RenderAudioContent();
          break;
        case CardFieldType::Image:
          RenderImageContent(renderer);
          break;
      }

      ImGui::PopID();
    }

    void CardField::RenderTypeSelector()
    {
      bool isText = (m_Type == CardFieldType::Text);
      bool isAudio = (m_Type == CardFieldType::Audio);
      bool isImage = (m_Type == CardFieldType::Image);

      if (isText)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
      if (ImGui::Button(ICON_FA_FONT))
        SetType(CardFieldType::Text);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Text");
      if (isText)
        ImGui::PopStyleColor();

      ImGui::SameLine();

      if (isAudio)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
      if (ImGui::Button(ICON_FA_MUSIC))
        SetType(CardFieldType::Audio);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Audio");
      if (isAudio)
        ImGui::PopStyleColor();

      ImGui::SameLine();

      if (isImage)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
      if (ImGui::Button(ICON_FA_IMAGE))
        SetType(CardFieldType::Image);
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Image");
      if (isImage)
        ImGui::PopStyleColor();
    }

    void CardField::RenderTextContent()
    {
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
      ImGui::InputTextMultiline("##Value", &m_Value, ImVec2(0, 60));
    }

    void CardField::RenderAudioContent()
    {
      auto RenderBrowseButton = [this]() {
        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Browse...")) {
          LoadAudioFile();
        }
      };

      auto RenderClearButton = [this]() {
        if (ImGui::Button(ICON_FA_TRASH " Clear")) {
          ClearContent();
        }
      };

      if (m_Value.empty()) {
        ImGui::TextDisabled("No audio loaded");
        RenderBrowseButton();
      } else {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("File: %s", m_Value.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu bytes)", m_BinaryData.size());

        if (ImGui::Button(ICON_FA_PLAY " Play")) {
          m_AudioPlayer->Play(m_BinaryData);
        }
        ImGui::SameLine();
        RenderBrowseButton();
        ImGui::SameLine();
        RenderClearButton();
      }
    }

    void CardField::RenderImageContent(SDL_Renderer* renderer)
    {
      auto UpdateTexture = [this, renderer]() {
        if (!m_TextureNeedsUpdate || !renderer)
          return;

        if (m_ImageTexture) {
          SDL_DestroyTexture(m_ImageTexture);
          m_ImageTexture = nullptr;
        }

        if (!m_BinaryData.empty()) {
          int width{}, height{}, channels{};
          unsigned char* data = stbi_load_from_memory(
              m_BinaryData.data(), static_cast<int>(m_BinaryData.size()), &width, &height, &channels, 4);

          bool isWebP = false;
          if (!data) {
            // Try WebP
            data = WebPDecodeRGBA(m_BinaryData.data(), m_BinaryData.size(), &width, &height);
            if (data) {
              isWebP = true;
            }
          }

          if (data) {
            auto surface = SDL::MakeSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, width * 4);

            if (surface) {
              m_ImageTexture = SDL_CreateTextureFromSurface(renderer, surface.get());
              m_ImageWidth = width;
              m_ImageHeight = height;
            }

            if (isWebP) {
              WebPFree(data);
            } else {
              stbi_image_free(data);
            }
          }
        }
        m_TextureNeedsUpdate = false;
      };

      auto RenderBrowseButton = [this]() {
        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Browse...")) {
          LoadImageFile();
        }
      };

      auto RenderClearButton = [this]() {
        if (ImGui::Button(ICON_FA_TRASH " Clear")) {
          ClearContent();
        }
      };

      UpdateTexture();

      if (m_ImageTexture) {
        const float aspect = static_cast<float>(m_ImageWidth) / static_cast<float>(m_ImageHeight);
        constexpr float thumbHeight = 100.0f;
        const float thumbWidth = thumbHeight * aspect;

        ImGui::Image(m_ImageTexture, ImVec2(thumbWidth, thumbHeight));
        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::Text("File: %s", m_Value.c_str());
        RenderBrowseButton();
        ImGui::SameLine();
        RenderClearButton();
        ImGui::EndGroup();
      } else {
        ImGui::TextDisabled("No image loaded");
        RenderBrowseButton();
      }
    }

    void CardField::SetValue(const std::string& value)
    {
      m_Value = value;
    }

    void CardField::SetBinaryData(const std::vector<unsigned char>& data, const std::string& filename)
    {
      m_BinaryData = data;
      m_Value = filename;
      if (m_Type == CardFieldType::Image) {
        m_TextureNeedsUpdate = true;
      }
    }

    void CardField::SetType(CardFieldType type)
    {
      if (m_Type != type) {
        m_Type = type;
      }
    }

    void CardField::ClearContent()
    {
      m_Value.clear();
      m_BinaryData.clear();

      if (m_ImageTexture) {
        SDL_DestroyTexture(m_ImageTexture);
        m_ImageTexture = nullptr;
      }

      if (m_AudioPlayer) {
        m_AudioPlayer->Stop();
      }
    }

    namespace
    {
      std::optional<std::vector<unsigned char>> LoadFileToMemory(const std::filesystem::path& filepath)
      {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
          AF_ERROR("Failed to open file: {}", filepath.string());
          return std::nullopt;
        }

        const auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
          AF_ERROR("Failed to read file: {}", filepath.string());
          return std::nullopt;
        }

        return buffer;
      }

      void OnFileSelected(void* userdata, const char* const* filelist, int /*filter*/)
      {
        if (!filelist || !filelist[0])
          return;

        auto* field = static_cast<CardField*>(userdata);
        const std::filesystem::path filepath(filelist[0]);

        auto buffer = LoadFileToMemory(filepath);
        if (!buffer)
          return;

        const std::string filename = filepath.filename().string();
        field->SetBinaryData(*buffer, filename);
        AF_INFO("Loaded file: {} ({} bytes)", filename, buffer->size());
      }
    } // namespace

    void CardField::LoadAudioFile()
    {
      static const SDL_DialogFileFilter filter = {"Audio Files", "mp3;wav;ogg;m4a;flac"};

      SDL_ShowOpenFileDialog(OnFileSelected, this, nullptr, &filter, 1, nullptr, false);
    }

    void CardField::LoadImageFile()
    {
      static const SDL_DialogFileFilter filter = {"Image Files", "png;jpg;jpeg;bmp;gif;webp"};

      SDL_ShowOpenFileDialog(OnFileSelected, this, nullptr, &filter, 1, nullptr, false);
    }

  } // namespace UI
} // namespace Video2Card

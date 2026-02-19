#pragma once

#include <imgui.h>

#include <functional>
#include <memory>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mutex>
#include <string>
#include <vector>

#include "ui/UIComponent.h"
#include "utils/VideoState.h"

struct SDL_Texture;
struct SDL_Renderer;

namespace Video2Card::Config
{
  class ConfigManager;
}

namespace Video2Card::Language
{
  class ILanguage;
}

namespace Video2Card::UI
{

  struct SubtitleData
  {
    std::string text;
    double start = 0.0;
    double end = 0.0;
  };

  class VideoSection : public UIComponent
  {
public:

    VideoSection(SDL_Renderer* renderer,
                 Config::ConfigManager* configManager,
                 std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                 Language::ILanguage** activeLanguage);
    ~VideoSection() override;

    void Render() override;
    void Update() override;

    void SetOnExtractCallback(std::function<void()> callback) { m_OnExtractCallback = callback; }

    void LoadVideoFromFile(const std::string& path);
    void ClearVideo();

    // Media Controls
    void TogglePlayback();
    void Seek(double seconds); // Relative seek
    void SeekAbsolute(double timestamp);

    // Extraction
    std::vector<unsigned char> GetCurrentFrameImage();
    SubtitleData GetCurrentSubtitle();
    std::vector<unsigned char> GetAudioClip(double start, double end);
    double GetCurrentTimestamp();

    int GetSubtitleOffsetMs() const { return m_SubtitleOffsetMs; }
    void SetSubtitleOffsetMs(int offsetMs) { m_SubtitleOffsetMs = offsetMs; }

private:

    void InitializeMPV();
    void DestroyMPV();
    void HandleMPVEvents();
    void UpdateVideoTexture();
    void DrawControls();

    SDL_Renderer* m_Renderer;
    Config::ConfigManager* m_ConfigManager;
    std::vector<std::unique_ptr<Language::ILanguage>>* m_Languages;
    Language::ILanguage** m_ActiveLanguage;

    mpv_handle* m_mpv = nullptr;
    mpv_render_context* m_mpv_render = nullptr;

    SDL_Texture* m_VideoTexture = nullptr;
    int m_VideoWidth = 0;
    int m_VideoHeight = 0;

    std::string m_CurrentVideoPath;
    bool m_IsPlaying = false;
    double m_Duration = 0.0;
    double m_CurrentTime = 0.0;
    double m_Volume = 100.0;

    std::function<void()> m_OnExtractCallback;

    // Buffer for software rendering from MPV
    std::vector<uint8_t> m_FrameBuffer;

    bool m_ShouldClearVideo = false;
    double m_LastSaveTime = 0.0;
    static constexpr double SAVE_INTERVAL = 1.0;

    double m_PendingSeekPosition = -1.0;
    bool m_FileLoadedSuccessfully = false;

    int m_SubtitleOffsetMs = 0;
  };

} // namespace Video2Card::UI

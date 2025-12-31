#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;

namespace Video2Card::UI
{
  class VideoSection;
  class ConfigurationSection;
  class AnkiCardSettingsSection;
  class StatusSection;
} // namespace Video2Card::UI

namespace Video2Card::API
{
  class AnkiConnectClient;
}

namespace Video2Card::Language::Services
{
  class ILanguageService;
}

namespace Video2Card::Language
{
  class ILanguage;
}

namespace Video2Card::Language::Analyzer
{
  class SentenceAnalyzer;
}

namespace Video2Card::Language::Audio
{
  class ForvoClient;
}

namespace Video2Card::Config
{
  class ConfigManager;
}

namespace Video2Card
{

  class Application
  {
public:

    Application(std::string title, int width, int height);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Run();

private:

    bool Initialize();
    void Shutdown();

    void HandleEvents();
    void Update();
    void Render();
    void RenderUI();

    void OnExtract();
    void RenderExtractModal();
    void ProcessExtract();

    std::string HighlightTargetWord(const std::string& text, const std::string& targetWord);

    void SaveWindowState();
    void LoadWindowState();

    void UpdateAsyncTasks();
    void CancelAsyncTasks();

    std::string m_Title;
    int m_Width;
    int m_Height;
    bool m_IsRunning;

    SDL_Window* m_Window;
    SDL_Renderer* m_Renderer;

    std::string m_BasePath;

    std::unique_ptr<UI::VideoSection> m_VideoSection;
    std::unique_ptr<UI::ConfigurationSection> m_ConfigurationSection;
    std::unique_ptr<UI::AnkiCardSettingsSection> m_AnkiCardSettingsSection;
    std::unique_ptr<UI::StatusSection> m_StatusSection;

    std::unique_ptr<API::AnkiConnectClient> m_AnkiConnectClient;
    std::unique_ptr<Config::ConfigManager> m_ConfigManager;

    std::vector<std::unique_ptr<Language::Services::ILanguageService>> m_LanguageServices;
    std::unique_ptr<Language::Analyzer::SentenceAnalyzer> m_SentenceAnalyzer;
    std::unique_ptr<Language::Audio::ForvoClient> m_ForvoClient;

    std::vector<std::unique_ptr<Language::ILanguage>> m_Languages;
    Language::ILanguage* m_ActiveLanguage = nullptr;

    bool m_ShowExtractModal = false;
    bool m_OpenExtractModal = false;
    std::string m_ExtractSentence;
    std::string m_ExtractTargetWord;

    // Data extracted from video for the card
    std::vector<unsigned char> m_ExtractedImage;
    std::vector<unsigned char> m_ExtractedAudio;

    struct AsyncTask
    {
      std::future<void> future;
      std::string description;
      std::function<void()> onComplete;
      std::function<void(const std::string&)> onError;
    };

    std::queue<AsyncTask> m_ActiveTasks;
    std::mutex m_TaskMutex;

    std::atomic<bool> m_IsExtracting{false};
    std::atomic<bool> m_IsProcessing{false};
    std::atomic<bool> m_CancelRequested{false};
    std::atomic<bool> m_AnkiConnected{false};

    std::mutex m_ResultMutex;
    std::string m_LastError;
  };

} // namespace Video2Card

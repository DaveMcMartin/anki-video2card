#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace Video2Card::Config
{

  struct AppConfig
  {
    std::string AnkiConnectUrl = "http://localhost:8765";
    std::vector<std::string> AnkiDecks;
    std::vector<std::string> AnkiNoteTypes;

    std::string SelectedLanguage = "JP";

    std::string TextProvider = "xAI";
    std::string TextApiKey;
    std::string TextVisionModel = "grok-2-vision-1212";
    std::string TextSentenceModel = "grok-2-1212";
    std::vector<std::string> TextAvailableModels;

    std::string GoogleApiKey;
    std::string GoogleVisionModel = "gemini-2.0-flash";
    std::string GoogleSentenceModel = "gemini-2.0-flash";
    std::vector<std::string> GoogleAvailableModels;

    std::string AudioProvider = "ElevenLabs";
    std::string AudioApiKey;
    std::string AudioVoiceId;
    std::vector<std::pair<std::string, std::string>> AudioAvailableVoices;

    // Audio Configuration
    std::string AudioFormat = "mp3"; // "mp3" or "opus"

    // DeepL Translation Configuration
    std::string DeepLApiKey;
    bool DeepLUseFreeAPI = true;
    std::string DeepLSourceLang = "JA";
    std::string DeepLTargetLang = "EN-US";

    int WindowWidth = 1280;
    int WindowHeight = 720;

    std::string LastNoteType;
    std::string LastDeck;
    std::map<std::string, std::map<std::string, std::pair<bool, int>>> FieldMappings;
  };

  class ConfigManager
  {
public:

    explicit ConfigManager(std::string configPath = "config.json");
    ~ConfigManager() = default;

    void Load();
    void Save();

    AppConfig& GetConfig();

private:

    std::string m_ConfigPath;
    AppConfig m_Config;
  };

} // namespace Video2Card::Config

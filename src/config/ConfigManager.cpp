#include "config/ConfigManager.h"

#include <fstream>
#include <iostream>

#include "core/Logger.h"

namespace Video2Card::Config
{

  ConfigManager::ConfigManager(std::string configPath)
      : m_ConfigPath(std::move(configPath))
  {
    Load();
  }

  void ConfigManager::Load()
  {
    std::ifstream file(m_ConfigPath);
    if (!file.is_open()) {
      return;
    }

    try {
      nlohmann::json j;
      file >> j;

      if (j.contains("anki_connect_url"))
        m_Config.AnkiConnectUrl = j["anki_connect_url"];
      if (j.contains("anki_decks"))
        m_Config.AnkiDecks = j["anki_decks"].get<std::vector<std::string>>();
      if (j.contains("anki_note_types"))
        m_Config.AnkiNoteTypes = j["anki_note_types"].get<std::vector<std::string>>();

      if (j.contains("selected_language"))
        m_Config.SelectedLanguage = j["selected_language"];

      if (j.contains("text_provider"))
        m_Config.TextProvider = j["text_provider"];
      if (j.contains("text_api_key"))
        m_Config.TextApiKey = j["text_api_key"];
      if (j.contains("text_vision_model"))
        m_Config.TextVisionModel = j["text_vision_model"];
      if (j.contains("text_sentence_model"))
        m_Config.TextSentenceModel = j["text_sentence_model"];
      if (j.contains("text_available_models"))
        m_Config.TextAvailableModels = j["text_available_models"].get<std::vector<std::string>>();

      if (j.contains("google_api_key"))
        m_Config.GoogleApiKey = j["google_api_key"];
      if (j.contains("google_vision_model"))
        m_Config.GoogleVisionModel = j["google_vision_model"];
      if (j.contains("google_sentence_model"))
        m_Config.GoogleSentenceModel = j["google_sentence_model"];
      if (j.contains("google_model"))
        m_Config.GoogleVisionModel = j["google_model"]; // Fallback for old config
      if (j.contains("google_available_models"))
        m_Config.GoogleAvailableModels = j["google_available_models"].get<std::vector<std::string>>();

      if (j.contains("audio_provider"))
        m_Config.AudioProvider = j["audio_provider"];
      if (j.contains("audio_api_key"))
        m_Config.AudioApiKey = j["audio_api_key"];
      if (j.contains("audio_voice_id"))
        m_Config.AudioVoiceId = j["audio_voice_id"];
      if (j.contains("audio_available_voices")) {
        m_Config.AudioAvailableVoices.clear();
        for (const auto& item : j["audio_available_voices"]) {
          if (item.is_array() && item.size() == 2) {
            m_Config.AudioAvailableVoices.push_back({item[0], item[1]});
          }
        }
      }

      if (j.contains("audio_format"))
        m_Config.AudioFormat = j["audio_format"];

      if (j.contains("last_note_type"))
        m_Config.LastNoteType = j["last_note_type"];
      if (j.contains("last_deck"))
        m_Config.LastDeck = j["last_deck"];

      if (j.contains("field_mappings")) {
        m_Config.FieldMappings.clear();
        for (auto& [noteType, fields] : j["field_mappings"].items()) {
          for (auto& [fieldName, settings] : fields.items()) {
            if (settings.is_array() && settings.size() == 2) {
              m_Config.FieldMappings[noteType][fieldName] = {settings[0], settings[1]};
            }
          }
        }
      }
    } catch (const std::exception& e) {
      AF_ERROR("Error loading config: {}", e.what());
    }
  }

  void ConfigManager::Save()
  {
    nlohmann::json j;

    j["anki_connect_url"] = m_Config.AnkiConnectUrl;
    j["anki_decks"] = m_Config.AnkiDecks;
    j["anki_note_types"] = m_Config.AnkiNoteTypes;

    j["selected_language"] = m_Config.SelectedLanguage;

    j["text_provider"] = m_Config.TextProvider;
    j["text_api_key"] = m_Config.TextApiKey;
    j["text_vision_model"] = m_Config.TextVisionModel;
    j["text_sentence_model"] = m_Config.TextSentenceModel;
    j["text_available_models"] = m_Config.TextAvailableModels;

    j["google_api_key"] = m_Config.GoogleApiKey;
    j["google_vision_model"] = m_Config.GoogleVisionModel;
    j["google_sentence_model"] = m_Config.GoogleSentenceModel;
    j["google_available_models"] = m_Config.GoogleAvailableModels;

    j["audio_provider"] = m_Config.AudioProvider;
    j["audio_api_key"] = m_Config.AudioApiKey;
    j["audio_voice_id"] = m_Config.AudioVoiceId;

    nlohmann::json voicesJson = nlohmann::json::array();
    for (const auto& voice : m_Config.AudioAvailableVoices) {
      voicesJson.push_back({voice.first, voice.second});
    }
    j["audio_available_voices"] = voicesJson;

    j["audio_format"] = m_Config.AudioFormat;

    j["last_note_type"] = m_Config.LastNoteType;
    j["last_deck"] = m_Config.LastDeck;

    nlohmann::json mappingsJson;
    for (const auto& [noteType, fields] : m_Config.FieldMappings) {
      for (const auto& [fieldName, settings] : fields) {
        mappingsJson[noteType][fieldName] = {settings.first, settings.second};
      }
    }
    j["field_mappings"] = mappingsJson;

    std::ofstream file(m_ConfigPath);
    if (file.is_open()) {
      file << j.dump(4);
    } else {
      AF_ERROR("Error saving config to {}", m_ConfigPath);
    }
  }

  AppConfig& ConfigManager::GetConfig()
  {
    return m_Config;
  }

} // namespace Video2Card::Config

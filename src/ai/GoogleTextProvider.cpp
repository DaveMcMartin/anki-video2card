#include "ai/GoogleTextProvider.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <format>
#include <httplib.h>
#include <iostream>
#include <sstream>
#include <thread>

#include "core/Logger.h"
#include "language/ILanguage.h"
#include "utils/Base64Utils.h"

namespace Video2Card::AI
{

  GoogleTextProvider::GoogleTextProvider() {}

  GoogleTextProvider::~GoogleTextProvider() {}

  bool GoogleTextProvider::RenderConfigurationUI()
  {
    bool changed = false;

    if (ImGui::InputText("API Key", &m_ApiKey, ImGuiInputTextFlags_Password)) {
      changed = true;
    }

    if (m_IsLoadingModels) {
      if (ImGui::Button("Cancel")) {
        m_CancelLoadModels.store(true);
      }
      ImGui::SameLine();
      ImGui::Text("Loading...");
    } else {
      if (ImGui::Button("Load Models")) {
        m_CancelLoadModels.store(false);
        std::thread([this]() { LoadRemoteModels(); }).detach();
        changed = true;
      }
    }

    if (!m_StatusMessage.empty()) {
      ImGui::SameLine();
      ImGui::Text("%s", m_StatusMessage.c_str());
    }

    std::vector<std::string> displayModels = m_AvailableModels;

    if (ImGui::BeginCombo("Sentence Model", m_SentenceModel.c_str())) {
      for (const auto& model : displayModels) {
        bool is_selected = (m_SentenceModel == model);
        if (ImGui::Selectable(model.c_str(), is_selected)) {
          m_SentenceModel = model;
          changed = true;
        }
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    return changed;
  }

  void GoogleTextProvider::LoadConfig(const nlohmann::json& json)
  {
    if (json.contains("api_key"))
      m_ApiKey = json["api_key"];
    if (json.contains("sentence_model"))
      m_SentenceModel = json["sentence_model"];
    if (json.contains("available_models"))
      m_AvailableModels = json["available_models"].get<std::vector<std::string>>();
  }

  nlohmann::json GoogleTextProvider::SaveConfig() const
  {
    return {{"api_key", m_ApiKey},
            {"sentence_model", m_SentenceModel},
            {"available_models", m_AvailableModels}};
  }

  void GoogleTextProvider::LoadRemoteModels()
  {
    if (m_ApiKey.empty()) {
      m_StatusMessage = "Error: API Key required.";
      return;
    }

    m_IsLoadingModels = true;
    m_StatusMessage = "";

    try {
      httplib::Client cli("https://generativelanguage.googleapis.com");
      cli.set_connection_timeout(240);
      cli.set_read_timeout(240);

      std::string path = "/v1beta/models?key=" + m_ApiKey;
      auto res = cli.Get(path);

      if (m_CancelLoadModels.load()) {
        m_IsLoadingModels = false;
        m_StatusMessage = "Load cancelled.";
        return;
      }

      if (res && res->status == 200) {
        auto json = nlohmann::json::parse(res->body);
        std::vector<std::string> newModels;
        if (json.contains("models") && json["models"].is_array()) {
          for (const auto& item : json["models"]) {
            if (item.contains("name")) {
              std::string name = item["name"];
              if (name.rfind("models/", 0) == 0) {
                name = name.substr(7);
              }

              if (name.find("gemini") != std::string::npos) {
                newModels.push_back(name);
              }
            }
          }
        }
        m_AvailableModels = newModels;
        m_StatusMessage = "Models loaded.";
      } else {
        m_StatusMessage = "Error loading models: " + std::to_string(res ? res->status : 0);
      }
    } catch (const std::exception& e) {
      m_StatusMessage = std::string("Exception: ") + e.what();
    }

    m_IsLoadingModels = false;
  }

  nlohmann::json GoogleTextProvider::SendRequest(const std::string& endpoint, const nlohmann::json& payload)
  {
    if (m_ApiKey.empty()) {
      AF_ERROR("GoogleTextProvider Error: API Key is missing.");
      return nullptr;
    }

    try {
      httplib::Client cli("https://generativelanguage.googleapis.com");
      cli.set_connection_timeout(120);
      cli.set_read_timeout(120);

      std::string path = endpoint + "?key=" + m_ApiKey;

      httplib::Headers headers = {{"Content-Type", "application/json"}};

      auto res = cli.Post(path, headers, payload.dump(), "application/json");

      if (res && res->status == 200) {
        return nlohmann::json::parse(res->body);
      } else {
        AF_ERROR("GoogleTextProvider HTTP Error: {}", (res ? res->status : 0));
        if (res)
          AF_ERROR("Response: {}", res->body);
        return nullptr;
      }
    } catch (const std::exception& e) {
      AF_ERROR("GoogleTextProvider Exception: {}", e.what());
      return nullptr;
    }
  }

  nlohmann::json GoogleTextProvider::AnalyzeSentence(const std::string& sentence,
                                                     const std::string& targetWord,
                                                     Language::ILanguage* language)
  {
    if (!language)
      return nlohmann::json();

    std::string prompt = language->GetAnalysisUserPrompt(sentence, targetWord);

    nlohmann::json payload = {{"system_instruction", {{"parts", {{{"text", language->GetAnalysisSystemPrompt()}}}}}},
                              {"contents", {{{"role", "user"}, {"parts", {{{"text", prompt}}}}}}},
                              {"generationConfig", {{"temperature", 0.1}, {"response_mime_type", "application/json"}}}};

#ifndef NDEBUG
    AF_DEBUG("Sending Google Analysis Request: {}", payload.dump(2));
#endif

    std::string endpoint = "/v1beta/models/" + m_SentenceModel + ":generateContent";
    auto response = SendRequest(endpoint, payload);

    if (!response.is_null() && response.contains("candidates") && !response["candidates"].empty()) {
      auto& candidate = response["candidates"][0];
      if (candidate.contains("content") && candidate["content"].contains("parts") &&
          !candidate["content"]["parts"].empty())
      {
        std::string content = candidate["content"]["parts"][0]["text"].get<std::string>();
        AF_INFO("AnalyzeSentence Response Content: {}", content);

        try {
          return nlohmann::json::parse(content);
        } catch (const std::exception& e) {
          AF_ERROR("JSON Parse Error: {}\nContent: {}", e.what(), content);
        }
      }
    }

    return nullptr;
  }

} // namespace Video2Card::AI

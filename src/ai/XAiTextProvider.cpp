#include "ai/XAiTextProvider.h"

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

  XAiTextProvider::XAiTextProvider() {}

  XAiTextProvider::~XAiTextProvider() {}

  bool XAiTextProvider::RenderConfigurationUI()
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

  void XAiTextProvider::LoadConfig(const nlohmann::json& json)
  {
    if (json.contains("api_key"))
      m_ApiKey = json["api_key"];
    if (json.contains("sentence_model"))
      m_SentenceModel = json["sentence_model"];
    if (json.contains("available_models"))
      m_AvailableModels = json["available_models"].get<std::vector<std::string>>();
  }

  nlohmann::json XAiTextProvider::SaveConfig() const
  {
    return {{"api_key", m_ApiKey}, {"sentence_model", m_SentenceModel}, {"available_models", m_AvailableModels}};
  }

  void XAiTextProvider::LoadRemoteModels()
  {
    if (m_ApiKey.empty()) {
      m_StatusMessage = "Error: API Key required.";
      return;
    }

    m_IsLoadingModels = true;
    m_StatusMessage = "";

    try {
      httplib::Client cli("https://api.x.ai");
      cli.set_connection_timeout(120);
      cli.set_read_timeout(120);

      httplib::Headers headers = {{"Authorization", "Bearer " + m_ApiKey}};

      auto res = cli.Get("/v1/models", headers);

      if (m_CancelLoadModels.load()) {
        m_IsLoadingModels = false;
        m_StatusMessage = "Load cancelled.";
        return;
      }

      if (res && res->status == 200) {
        auto json = nlohmann::json::parse(res->body);
        m_AvailableModels.clear();
        if (json.contains("data") && json["data"].is_array()) {
          for (const auto& item : json["data"]) {
            if (item.contains("id")) {
              m_AvailableModels.push_back(item["id"]);
            }
          }
        }
        m_StatusMessage = "Models loaded.";
      } else {
        m_StatusMessage = "Error loading models: " + std::to_string(res ? res->status : 0);
      }
    } catch (const std::exception& e) {
      m_StatusMessage = std::string("Exception: ") + e.what();
    }

    m_IsLoadingModels = false;
  }

  nlohmann::json XAiTextProvider::SendRequest(const std::string& endpoint, const nlohmann::json& payload)
  {
    if (m_ApiKey.empty()) {
      AF_ERROR("XAiTextProvider Error: API Key is missing.");
      return nullptr;
    }

    try {
      httplib::Client cli("https://api.x.ai");
      cli.set_connection_timeout(120);
      cli.set_read_timeout(120);

      httplib::Headers headers = {{"Authorization", "Bearer " + m_ApiKey}, {"Content-Type", "application/json"}};

      auto res = cli.Post(endpoint, headers, payload.dump(), "application/json");

      if (res && res->status == 200) {
        return nlohmann::json::parse(res->body);
      } else {
        AF_ERROR("XAiTextProvider HTTP Error: {}", (res ? res->status : 0));
        if (res)
          AF_ERROR("Response: {}", res->body);
        return nullptr;
      }
    } catch (const std::exception& e) {
      AF_ERROR("XAiTextProvider Exception: {}", e.what());
      return nullptr;
    }
  }

  nlohmann::json XAiTextProvider::AnalyzeSentence(const std::string& sentence,
                                                  const std::string& targetWord,
                                                  Language::ILanguage* language)
  {
    if (!language)
      return nlohmann::json();

    std::string prompt = language->GetAnalysisUserPrompt(sentence, targetWord);

    AF_INFO("AnalyzeSentence Prompt: {}", prompt);

    nlohmann::json payload = {
        {"model", m_SentenceModel},
        {"messages",
         nlohmann::json::array({{{"role", "system"}, {"content", language->GetAnalysisSystemPrompt()}},
                                {{"role", "user"}, {"content", prompt}}})},
        {"temperature", 0.1},
        {"stream", false}};

#ifndef NDEBUG
    AF_DEBUG("Sending Analysis Request: {}", payload.dump(2));
#endif

    auto response = SendRequest("/v1/chat/completions", payload);
    if (!response.is_null() && response.contains("choices") && !response["choices"].empty()) {
      if (response.contains("usage")) {
        AF_INFO("xAI Token Usage (Analysis): Prompt={}, Completion={}, Total={}",
                response["usage"].value("prompt_tokens", 0),
                response["usage"].value("completion_tokens", 0),
                response["usage"].value("total_tokens", 0));
      }

      std::string content = response["choices"][0]["message"]["content"].get<std::string>();
      AF_INFO("AnalyzeSentence Response Content: {}", content);

      size_t jsonStart = content.find("{");
      size_t jsonEnd = content.rfind("}");

      if (jsonStart != std::string::npos && jsonEnd != std::string::npos) {
        content = content.substr(jsonStart, jsonEnd - jsonStart + 1);
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

#include "ui/ConfigurationSection.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <thread>

#include "api/AnkiConnectClient.h"
#include "config/ConfigManager.h"
#include "core/Logger.h"
#include "language/ILanguage.h"
#include "language/services/ILanguageService.h"

namespace Video2Card::UI
{

  ConfigurationSection::ConfigurationSection(
      API::AnkiConnectClient* ankiConnectClient,
      Config::ConfigManager* configManager,
      std::vector<std::unique_ptr<Language::Services::ILanguageService>>* languageServices,
      std::vector<std::unique_ptr<Language::ILanguage>>* languages,
      Language::ILanguage** activeLanguage)
      : m_AnkiConnectClient(ankiConnectClient)
      , m_ConfigManager(configManager)
      , m_LanguageServices(languageServices)
      , m_Languages(languages)
      , m_ActiveLanguage(activeLanguage)
  {}

  ConfigurationSection::~ConfigurationSection() {}

  void ConfigurationSection::Render()
  {
    // Not used - tabs are rendered directly in Application.cpp
  }

  void ConfigurationSection::RenderAnkiConnectTab()
  {
    ImGui::Spacing();
    ImGui::Text("AnkiConnect Configuration");
    ImGui::Separator();
    ImGui::Spacing();

    if (!m_ConfigManager)
      return;
    auto& config = m_ConfigManager->GetConfig();

    if (ImGui::InputText("URL", &config.AnkiConnectUrl)) {
      m_ConfigManager->Save();
    }

    ImGui::Spacing();

    if (ImGui::Button("Connect")) {
      m_AnkiConnectError.clear();
      if (m_AnkiConnectClient) {
        std::thread([this, url = config.AnkiConnectUrl]() {
          m_AnkiConnectClient->SetUrl(url);
          m_AnkiConnectConnected = m_AnkiConnectClient->Ping();
          if (m_AnkiConnectConnected) {
            if (m_OnConnectCallback) {
              m_OnConnectCallback();
            }
          } else {
            m_AnkiConnectError = "Connection failed. Ensure Anki is open and AnkiConnect is installed.";
          }
        }).detach();
      }
    }

    ImGui::SameLine();
    if (m_AnkiConnectConnected) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected");
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");
    }

    if (!m_AnkiConnectError.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", m_AnkiConnectError.c_str());
    }
  }

  void ConfigurationSection::RenderLanguageServicesTab()
  {
    ImGui::Spacing();

    if (!m_LanguageServices || !m_ConfigManager)
      return;

    auto& config = m_ConfigManager->GetConfig();

    // Group services by type
    std::vector<Language::Services::ILanguageService*> translators;

    for (auto& service : *m_LanguageServices) {
      std::string type = service->GetType();
      if (type == "translator") {
        translators.push_back(service.get());
      }
    }

    // Provider selection
    if (!translators.empty()) {
      if (m_SelectedTranslatorIndex >= static_cast<int>(translators.size())) {
        m_SelectedTranslatorIndex = 0;
      }

      Language::Services::ILanguageService* selectedTranslator = translators[m_SelectedTranslatorIndex];

      ImGui::Text("Provider");
      ImGui::SetNextItemWidth(-1);
      if (ImGui::BeginCombo("##TranslationProvider",
                            selectedTranslator ? selectedTranslator->GetName().c_str() : "None"))
      {
        for (size_t i = 0; i < translators.size(); i++) {
          bool isSelected = (static_cast<int>(i) == m_SelectedTranslatorIndex);
          if (ImGui::Selectable(translators[i]->GetName().c_str(), isSelected)) {
            m_SelectedTranslatorIndex = static_cast<int>(i);
            selectedTranslator = translators[i];
            if (m_OnTranslatorChangeCallback) {
              m_OnTranslatorChangeCallback(selectedTranslator->GetId());
            }
          }
          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Render selected translator's configuration
      if (selectedTranslator) {
        if (selectedTranslator->RenderConfigurationUI()) {
          // Save config for this service
          auto serviceConfig = selectedTranslator->SaveConfig();

          if (selectedTranslator->GetId() == "deepl") {
            if (serviceConfig.contains("api_key"))
              config.DeepLApiKey = serviceConfig["api_key"];
            if (serviceConfig.contains("use_free_api"))
              config.DeepLUseFreeAPI = serviceConfig["use_free_api"];
            if (serviceConfig.contains("source_lang"))
              config.DeepLSourceLang = serviceConfig["source_lang"];
            if (serviceConfig.contains("target_lang"))
              config.DeepLTargetLang = serviceConfig["target_lang"];
          }

          m_ConfigManager->Save();
        }
      }
    }
  }

} // namespace Video2Card::UI

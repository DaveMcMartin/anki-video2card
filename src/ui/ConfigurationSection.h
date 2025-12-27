#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ui/UIComponent.h"

namespace Video2Card::API
{
  class AnkiConnectClient;
}

namespace Video2Card::Config
{
  class ConfigManager;
}

namespace Video2Card::Language::Services
{
  class ILanguageService;
}

namespace Video2Card::Language
{
  class ILanguage;
}

namespace Video2Card::UI
{

  class ConfigurationSection : public UIComponent
  {
public:

    ConfigurationSection(API::AnkiConnectClient* ankiConnectClient,
                         Config::ConfigManager* configManager,
                         std::vector<std::unique_ptr<Language::Services::ILanguageService>>* languageServices,
                         std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                         Language::ILanguage** activeLanguage);
    ~ConfigurationSection() override;

    void Render() override;

    void SetOnConnectCallback(std::function<void()> callback) { m_OnConnectCallback = callback; }

    void RenderAnkiConnectTab();
    void RenderLanguageServicesTab();

private:

    // AnkiConnect State
    bool m_AnkiConnectConnected = false;
    std::string m_AnkiConnectError;

    API::AnkiConnectClient* m_AnkiConnectClient;
    Config::ConfigManager* m_ConfigManager;
    std::vector<std::unique_ptr<Language::Services::ILanguageService>>* m_LanguageServices;
    std::vector<std::unique_ptr<Language::ILanguage>>* m_Languages;
    Language::ILanguage** m_ActiveLanguage;

    std::function<void()> m_OnConnectCallback;
  };

} // namespace Video2Card::UI

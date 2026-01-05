#include "GoogleTranslateService.h"

#include "core/Logger.h"

namespace Video2Card::Language::Services
{

  GoogleTranslateService::GoogleTranslateService()
      : m_SourceLang("ja")
      , m_TargetLang("en")
  {
    m_Translator = std::make_shared<Translation::GoogleTranslateTranslator>(m_SourceLang, m_TargetLang);
    AF_INFO("GoogleTranslateService created with source: {}, target: {}", m_SourceLang, m_TargetLang);
  }

  std::string GoogleTranslateService::GetName() const
  {
    return "Google Translate";
  }

  std::string GoogleTranslateService::GetId() const
  {
    return "google_translate";
  }

  std::string GoogleTranslateService::GetType() const
  {
    return "translator";
  }

  bool GoogleTranslateService::IsAvailable() const
  {
    bool available = m_Translator && m_Translator->IsAvailable();
    AF_DEBUG("GoogleTranslateService::IsAvailable() = {}", available);
    return available;
  }

  bool GoogleTranslateService::RenderConfigurationUI()
  {
    return false;
  }

  void GoogleTranslateService::LoadConfig(const nlohmann::json& config)
  {
    if (config.contains("source_lang") && config["source_lang"].is_string()) {
      m_SourceLang = config["source_lang"].get<std::string>();
    }

    if (config.contains("target_lang") && config["target_lang"].is_string()) {
      m_TargetLang = config["target_lang"].get<std::string>();
    }

    if (m_Translator) {
      m_Translator->SetSourceLang(m_SourceLang);
      m_Translator->SetTargetLang(m_TargetLang);
    }

    AF_INFO("GoogleTranslateService config loaded: source={}, target={}", m_SourceLang, m_TargetLang);
  }

  nlohmann::json GoogleTranslateService::SaveConfig() const
  {
    nlohmann::json config;
    config["source_lang"] = m_SourceLang;
    config["target_lang"] = m_TargetLang;
    return config;
  }

  std::shared_ptr<Translation::ITranslator> GoogleTranslateService::GetTranslator() const
  {
    return m_Translator;
  }

} // namespace Video2Card::Language::Services
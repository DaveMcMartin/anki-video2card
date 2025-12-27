#include "DeepLService.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "core/Logger.h"

namespace Video2Card::Language::Services
{

  DeepLService::DeepLService()
      : m_Translator(nullptr)
      , m_ApiKey("")
      , m_UseFreeAPI(true)
      , m_SourceLang("JA")
      , m_TargetLang("EN-US")
      , m_Formality("default")
      , m_TimeoutSeconds(10)
  {
    AF_INFO("DeepLService created");
  }

  std::string DeepLService::GetName() const
  {
    return "DeepL Translator";
  }

  std::string DeepLService::GetId() const
  {
    return "deepl";
  }

  std::string DeepLService::GetType() const
  {
    return "translator";
  }

  bool DeepLService::IsAvailable() const
  {
    return m_Translator && m_Translator->IsConfigured() && m_Translator->IsAvailable();
  }

  bool DeepLService::RenderConfigurationUI()
  {
    bool configChanged = false;

    // API Key input
    if (ImGui::InputText("API Key", &m_ApiKey, ImGuiInputTextFlags_Password)) {
      configChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Get API Key")) {
// Open DeepL API page in browser
#ifdef __APPLE__
      system("open https://www.deepl.com/pro-api");
#elif defined(_WIN32)
      system("start https://www.deepl.com/pro-api");
#else
      system("xdg-open https://www.deepl.com/pro-api");
#endif
    }

    ImGui::Spacing();

    // API tier selection
    if (ImGui::Checkbox("Use Free API (free tier)", &m_UseFreeAPI)) {
      configChanged = true;
    }
    ImGui::TextWrapped("Free API has a 500,000 character/month limit. Pro API requires a paid subscription.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Language settings
    ImGui::Text("Language Settings");

    const char* sourceLangs[] = {"JA", "EN", "DE", "FR", "ES", "IT", "NL", "PL", "PT", "RU", "ZH"};
    int currentSource = 0;
    for (int i = 0; i < IM_ARRAYSIZE(sourceLangs); i++) {
      if (m_SourceLang == sourceLangs[i]) {
        currentSource = i;
        break;
      }
    }

    if (ImGui::Combo("Source Language", &currentSource, sourceLangs, IM_ARRAYSIZE(sourceLangs))) {
      m_SourceLang = sourceLangs[currentSource];
      configChanged = true;
    }

    const char* targetLangs[] = {"EN-US", "EN-GB", "DE", "FR", "ES", "IT", "NL", "PL", "PT-BR", "PT-PT", "RU", "ZH"};
    int currentTarget = 0;
    for (int i = 0; i < IM_ARRAYSIZE(targetLangs); i++) {
      if (m_TargetLang == targetLangs[i]) {
        currentTarget = i;
        break;
      }
    }

    if (ImGui::Combo("Target Language", &currentTarget, targetLangs, IM_ARRAYSIZE(targetLangs))) {
      m_TargetLang = targetLangs[currentTarget];
      configChanged = true;
    }

    ImGui::Spacing();

    // Formality setting
    const char* formalityLevels[] = {"default", "more", "less"};
    int currentFormality = 0;
    for (int i = 0; i < IM_ARRAYSIZE(formalityLevels); i++) {
      if (m_Formality == formalityLevels[i]) {
        currentFormality = i;
        break;
      }
    }

    if (ImGui::Combo("Formality", &currentFormality, formalityLevels, IM_ARRAYSIZE(formalityLevels))) {
      m_Formality = formalityLevels[currentFormality];
      configChanged = true;
    }
    ImGui::TextWrapped("Controls the formality level of the translation.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Timeout setting
    if (ImGui::SliderInt("Timeout (seconds)", &m_TimeoutSeconds, 5, 30)) {
      configChanged = true;
    }

    ImGui::Spacing();

    // Initialize/Test button
    if (ImGui::Button("Initialize Translator")) {
      if (InitializeTranslator()) {
        AF_INFO("DeepL translator initialized successfully");
      } else {
        AF_ERROR("Failed to initialize DeepL translator");
      }
    }

    ImGui::SameLine();

    // Status indicator
    if (IsAvailable()) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Ready");
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Initialized");
    }

    return configChanged;
  }

  void DeepLService::LoadConfig(const nlohmann::json& config)
  {
    if (config.contains("api_key"))
      m_ApiKey = config["api_key"].get<std::string>();

    if (config.contains("use_free_api"))
      m_UseFreeAPI = config["use_free_api"].get<bool>();

    if (config.contains("source_lang"))
      m_SourceLang = config["source_lang"].get<std::string>();

    if (config.contains("target_lang"))
      m_TargetLang = config["target_lang"].get<std::string>();

    if (config.contains("formality"))
      m_Formality = config["formality"].get<std::string>();

    if (config.contains("timeout_seconds"))
      m_TimeoutSeconds = config["timeout_seconds"].get<int>();

    // Auto-initialize if we have an API key
    if (!m_ApiKey.empty()) {
      InitializeTranslator();
    }
  }

  nlohmann::json DeepLService::SaveConfig() const
  {
    nlohmann::json config;
    config["api_key"] = m_ApiKey;
    config["use_free_api"] = m_UseFreeAPI;
    config["source_lang"] = m_SourceLang;
    config["target_lang"] = m_TargetLang;
    config["formality"] = m_Formality;
    config["timeout_seconds"] = m_TimeoutSeconds;
    return config;
  }

  std::shared_ptr<Translation::DeepLTranslator> DeepLService::GetTranslator() const
  {
    return m_Translator;
  }

  bool DeepLService::InitializeTranslator()
  {
    if (m_ApiKey.empty()) {
      AF_WARN("Cannot initialize DeepL translator: API key is empty");
      return false;
    }

    try {
      m_Translator = std::make_shared<Translation::DeepLTranslator>(m_ApiKey, m_UseFreeAPI, m_TimeoutSeconds);

      m_Translator->SetSourceLanguage(m_SourceLang);
      m_Translator->SetTargetLanguage(m_TargetLang);
      m_Translator->SetFormality(m_Formality);

      AF_INFO("DeepL translator initialized successfully");
      return true;

    } catch (const std::exception& e) {
      AF_WARN("Failed to initialize DeepL translator: {}", e.what());
      m_Translator = nullptr;
      return false;
    }
  }

} // namespace Video2Card::Language::Services
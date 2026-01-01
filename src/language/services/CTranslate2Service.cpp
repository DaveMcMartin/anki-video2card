#include "CTranslate2Service.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "core/Logger.h"

namespace Video2Card::Language::Services
{

  CTranslate2Service::CTranslate2Service()
      : m_Translator(nullptr)
      , m_ModelPath("")
      , m_Device("cpu")
      , m_BeamSize(4)
      , m_IsEnabled(true)
  {
    AF_INFO("CTranslate2Service created");
  }

  std::string CTranslate2Service::GetName() const
  {
    return "Local Translator (CTranslate2)";
  }

  std::string CTranslate2Service::GetId() const
  {
    return "ctranslate2";
  }

  std::string CTranslate2Service::GetType() const
  {
    return "translator";
  }

  bool CTranslate2Service::IsAvailable() const
  {
    return m_IsEnabled && m_Translator && m_Translator->IsAvailable();
  }

  bool CTranslate2Service::RenderConfigurationUI()
  {
    bool configChanged = false;

    if (ImGui::Checkbox("Enable Local Translation", &m_IsEnabled)) {
      configChanged = true;
    }

    ImGui::Spacing();
    ImGui::TextWrapped("Uses entai2965/sugoi-v4-ja-en-ctranslate2 model for offline Japanese-English translation.");
    ImGui::Spacing();

    if (!m_IsEnabled) {
      ImGui::BeginDisabled();
    }

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Model Settings");

    ImGui::Text("Model Path: %s", m_ModelPath.empty() ? "Not set" : m_ModelPath.c_str());

    if (m_ModelPath.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Model not found!");
      ImGui::TextWrapped("Run: python3 scripts/download_translation_model.py");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Advanced Settings");

    const char* devices[] = {"cpu", "cuda", "auto"};
    int currentDevice = 0;
    for (int i = 0; i < IM_ARRAYSIZE(devices); i++) {
      if (m_Device == devices[i]) {
        currentDevice = i;
        break;
      }
    }

    if (ImGui::Combo("Device", &currentDevice, devices, IM_ARRAYSIZE(devices))) {
      m_Device = devices[currentDevice];
      configChanged = true;
    }
    ImGui::TextWrapped("Device to run inference on. Use 'cpu' for compatibility, 'cuda' for GPU acceleration.");

    ImGui::Spacing();

    int beamSizeInt = static_cast<int>(m_BeamSize);
    if (ImGui::SliderInt("Beam Size", &beamSizeInt, 1, 8)) {
      m_BeamSize = static_cast<size_t>(beamSizeInt);
      if (m_Translator && m_Translator->IsAvailable()) {
        m_Translator->SetBeamSize(m_BeamSize);
        AF_INFO("CTranslate2Service: Beam size updated to {}", m_BeamSize);
      }
      configChanged = true;
    }
    ImGui::TextWrapped("Higher values may improve quality but are slower. Default: 4");

    ImGui::Spacing();

    if (IsAvailable()) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Ready");
    } else if (!m_ModelPath.empty() && m_Translator) {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to Load");
    } else if (m_ModelPath.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Model Not Downloaded");
    } else {
      ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Not Initialized");
    }

    if (!m_IsEnabled) {
      ImGui::EndDisabled();
    }

    return configChanged;
  }

  void CTranslate2Service::LoadConfig(const nlohmann::json& config)
  {
    if (config.contains("enabled")) {
      m_IsEnabled = config["enabled"].get<bool>();
    }

    if (config.contains("device")) {
      m_Device = config["device"].get<std::string>();
    }

    if (config.contains("beam_size")) {
      m_BeamSize = config["beam_size"].get<size_t>();
      if (m_Translator && m_Translator->IsAvailable()) {
        m_Translator->SetBeamSize(m_BeamSize);
      }
    }
  }

  nlohmann::json CTranslate2Service::SaveConfig() const
  {
    nlohmann::json config;
    config["enabled"] = m_IsEnabled;
    config["device"] = m_Device;
    config["beam_size"] = m_BeamSize;
    return config;
  }

  std::shared_ptr<Translation::CTranslate2Translator> CTranslate2Service::GetTranslator() const
  {
    return m_Translator;
  }

  bool CTranslate2Service::InitializeTranslator(const std::string& basePath)
  {
    if (!m_IsEnabled) {
      AF_INFO("CTranslate2 translator is disabled");
      return false;
    }

    m_ModelPath = basePath + "assets/translation_model";

    try {
      m_Translator = std::make_shared<Translation::CTranslate2Translator>(m_ModelPath);

      if (m_Translator->IsAvailable()) {
        m_Translator->SetDevice(m_Device);
        m_Translator->SetBeamSize(m_BeamSize);

        AF_INFO("CTranslate2 translator initialized successfully from: {}", m_ModelPath);
        return true;
      } else {
        AF_WARN("CTranslate2 translator failed to initialize");
        m_Translator = nullptr;
        return false;
      }

    } catch (const std::exception& e) {
      AF_ERROR("Failed to initialize CTranslate2 translator: {}", e.what());
      m_Translator = nullptr;
      return false;
    }
  }

} // namespace Video2Card::Language::Services
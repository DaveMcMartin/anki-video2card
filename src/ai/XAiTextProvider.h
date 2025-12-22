#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "ai/ITextAIProvider.h"

namespace Video2Card::AI
{

  class XAiTextProvider : public ITextAIProvider
  {
public:

    XAiTextProvider();
    ~XAiTextProvider() override;

    std::string GetName() const override { return "xAI (Grok)"; }
    std::string GetId() const override { return "xai"; }

    bool RenderConfigurationUI() override;

    void LoadConfig(const nlohmann::json& json) override;
    nlohmann::json SaveConfig() const override;

    void LoadRemoteModels() override;

    nlohmann::json
    AnalyzeSentence(const std::string& sentence, const std::string& targetWord, Language::ILanguage* language) override;

private:

    nlohmann::json SendRequest(const std::string& endpoint, const nlohmann::json& payload);

    std::string m_ApiKey;
    std::string m_SentenceModel = "grok-2-1212";

    std::vector<std::string> m_AvailableModels;
    bool m_IsLoadingModels = false;
    std::string m_StatusMessage;
    std::atomic<bool> m_CancelLoadVoices{false};
    std::atomic<bool> m_CancelLoadModels{false};
  };

} // namespace Video2Card::AI

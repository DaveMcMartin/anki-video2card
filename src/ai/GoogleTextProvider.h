#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "ai/ITextAIProvider.h"

namespace Video2Card::AI
{

  class GoogleTextProvider : public ITextAIProvider
  {
public:

    GoogleTextProvider();
    ~GoogleTextProvider() override;

    std::string GetName() const override { return "Google (Gemini)"; }
    std::string GetId() const override { return "google"; }

    bool RenderConfigurationUI() override;

    void LoadConfig(const nlohmann::json& json) override;
    nlohmann::json SaveConfig() const override;

    void LoadRemoteModels() override;

    nlohmann::json
    AnalyzeSentence(const std::string& sentence, const std::string& targetWord, Language::ILanguage* language) override;

private:

    nlohmann::json SendRequest(const std::string& endpoint, const nlohmann::json& payload);

    std::string m_ApiKey;
    std::string m_SentenceModel = "gemini-2.0-flash";

    std::vector<std::string> m_AvailableModels;
    bool m_IsLoadingModels = false;
    std::string m_StatusMessage;
    std::atomic<bool> m_CancelLoadModels{false};
  };

} // namespace Video2Card::AI

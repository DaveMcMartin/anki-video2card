#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Video2Card::Language
{
  class ILanguage;
}

namespace Video2Card::AI
{

  class ITextAIProvider
  {
public:

    virtual ~ITextAIProvider() = default;

    virtual std::string GetName() const = 0;
    virtual std::string GetId() const = 0;

    virtual bool RenderConfigurationUI() = 0;

    virtual void LoadConfig(const nlohmann::json& json) = 0;
    virtual nlohmann::json SaveConfig() const = 0;

    virtual void LoadRemoteModels() = 0;

    virtual nlohmann::json
    AnalyzeSentence(const std::string& sentence, const std::string& targetWord, Language::ILanguage* language) = 0;
  };

} // namespace Video2Card::AI

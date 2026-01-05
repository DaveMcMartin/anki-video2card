#pragma once

#include <memory>
#include <string>

#include "language/services/ILanguageService.h"
#include "language/translation/GoogleTranslateTranslator.h"

namespace Video2Card::Language::Services
{

  class GoogleTranslateService : public ILanguageService
  {
public:

    GoogleTranslateService();
    ~GoogleTranslateService() override = default;

    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] std::string GetId() const override;
    [[nodiscard]] std::string GetType() const override;
    [[nodiscard]] bool IsAvailable() const override;

    bool RenderConfigurationUI() override;

    void LoadConfig(const nlohmann::json& config) override;
    [[nodiscard]] nlohmann::json SaveConfig() const override;

    [[nodiscard]] std::shared_ptr<Translation::ITranslator> GetTranslator() const;

private:

    std::shared_ptr<Translation::GoogleTranslateTranslator> m_Translator;

    std::string m_SourceLang;
    std::string m_TargetLang;
  };

} // namespace Video2Card::Language::Services
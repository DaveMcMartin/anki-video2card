#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "ILanguageService.h"
#include "language/translation/CTranslate2Translator.h"

namespace Video2Card::Language::Services
{

  class CTranslate2Service : public ILanguageService
  {
public:

    CTranslate2Service();
    ~CTranslate2Service() override = default;

    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] std::string GetId() const override;
    [[nodiscard]] std::string GetType() const override;
    [[nodiscard]] bool IsAvailable() const override;

    bool RenderConfigurationUI() override;
    void LoadConfig(const nlohmann::json& config) override;
    [[nodiscard]] nlohmann::json SaveConfig() const override;

    [[nodiscard]] std::shared_ptr<Translation::CTranslate2Translator> GetTranslator() const;

    bool InitializeTranslator(const std::string& basePath);

private:

    std::shared_ptr<Translation::CTranslate2Translator> m_Translator;
    std::string m_ModelPath;
    std::string m_Device;
    size_t m_BeamSize;
    bool m_IsEnabled;
  };

} // namespace Video2Card::Language::Services
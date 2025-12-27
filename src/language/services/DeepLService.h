#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "ILanguageService.h"
#include "language/translation/DeepLTranslator.h"

namespace Video2Card::Language::Services
{

  /**
 * DeepL translation service wrapper with UI configuration support.
 * Wraps the DeepLTranslator and provides UI configuration interface.
 */
  class DeepLService : public ILanguageService
  {
public:

    DeepLService();
    ~DeepLService() override = default;

    // ILanguageService interface
    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] std::string GetId() const override;
    [[nodiscard]] std::string GetType() const override;
    [[nodiscard]] bool IsAvailable() const override;

    bool RenderConfigurationUI() override;
    void LoadConfig(const nlohmann::json& config) override;
    [[nodiscard]] nlohmann::json SaveConfig() const override;

    /**
   * Get the underlying DeepL translator instance.
   * @return Shared pointer to translator, or nullptr if not initialized
   */
    [[nodiscard]] std::shared_ptr<Translation::DeepLTranslator> GetTranslator() const;

    /**
   * Initialize or reinitialize the translator with current settings.
   * Called automatically when API key is set.
   * @return true if initialization succeeded
   */
    bool InitializeTranslator();

private:

    std::shared_ptr<Translation::DeepLTranslator> m_Translator;

    // Configuration
    std::string m_ApiKey;
    bool m_UseFreeAPI;
    std::string m_SourceLang;
    std::string m_TargetLang;
    std::string m_Formality;
    int m_TimeoutSeconds;
  };

} // namespace Video2Card::Language::Services
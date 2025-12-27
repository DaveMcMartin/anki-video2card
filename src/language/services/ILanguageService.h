#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace Video2Card::Language::Services
{

  /**
 * Base interface for language services that can be configured via UI.
 * This includes dictionaries, translators, and other language tools.
 */
  class ILanguageService
  {
public:

    virtual ~ILanguageService() = default;

    /**
   * Get the human-readable name of this service.
   * @return Service name (e.g., "DeepL Translator", "Sakura-Paris Dictionary")
   */
    [[nodiscard]] virtual std::string GetName() const = 0;

    /**
   * Get a unique identifier for this service.
   * @return Service ID (e.g., "deepl", "sakura-paris")
   */
    [[nodiscard]] virtual std::string GetId() const = 0;

    /**
   * Get the type/category of this service.
   * @return Service type (e.g., "translator", "dictionary", "audio")
   */
    [[nodiscard]] virtual std::string GetType() const = 0;

    /**
   * Check if the service is currently available/initialized.
   * @return true if the service can be used
   */
    [[nodiscard]] virtual bool IsAvailable() const = 0;

    /**
   * Render ImGui configuration UI for this service.
   * @return true if configuration was changed and should be saved
   */
    virtual bool RenderConfigurationUI() = 0;

    /**
   * Load configuration from JSON.
   * @param config The configuration JSON object
   */
    virtual void LoadConfig(const nlohmann::json& config) = 0;

    /**
   * Save configuration to JSON.
   * @return Configuration JSON object
   */
    [[nodiscard]] virtual nlohmann::json SaveConfig() const = 0;
  };

} // namespace Video2Card::Language::Services
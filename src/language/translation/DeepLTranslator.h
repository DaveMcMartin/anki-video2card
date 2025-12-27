#pragma once

#include <memory>
#include <string>

#include "ITranslator.h"

namespace Video2Card::Language::Translation
{

  /**
 * DeepL API translator for Japanese to English translation.
 * Uses the DeepL API (free or pro tier) for high-quality translation.
 * 
 * If no API key is configured, the translator gracefully degrades by
 * returning the original text without translation.
 * 
 * API Documentation: https://www.deepl.com/docs-api
 */
  class DeepLTranslator : public ITranslator
  {
public:

    /**
   * Create a DeepL translator.
   * If apiKey is empty, the translator will be created in a disabled state
   * and will not attempt to make API calls.
   * 
   * @param apiKey DeepL API key (get from https://www.deepl.com/pro-api), can be empty
   * @param useFreeAPI If true, uses free API endpoint, otherwise uses Pro endpoint
   * @param timeoutSeconds HTTP request timeout in seconds
   */
    explicit DeepLTranslator(std::string apiKey, bool useFreeAPI = true, int timeoutSeconds = 10) noexcept;

    ~DeepLTranslator() override = default;

    /**
   * Translate Japanese text to English using DeepL.
   * If API is not configured, returns the original text unchanged.
   * @param text The Japanese text to translate
   * @return The English translation, or the original text if API is not available
   */
    [[nodiscard]] std::string Translate(const std::string& text) override;

    /**
   * Check if DeepL API is available.
   * Tests connectivity to the API endpoint.
   * Returns false if API key is not configured.
   * @return true if the API is accessible and configured
   */
    [[nodiscard]] bool IsAvailable() const override;

    /**
   * Check if the translator is configured (has an API key).
   * @return true if API key is set
   */
    [[nodiscard]] bool IsConfigured() const noexcept;

    /**
   * Set the source language (default: JA).
   * @param lang Language code (JA, EN, etc.)
   */
    void SetSourceLanguage(const std::string& lang) noexcept;

    /**
   * Set the target language (default: EN-US).
   * @param lang Language code (EN-US, EN-GB, etc.)
   */
    void SetTargetLanguage(const std::string& lang) noexcept;

    /**
   * Set formality level for translation.
   * @param formality "default", "more", or "less"
   */
    void SetFormality(const std::string& formality) noexcept;

private:

    /**
   * Get the appropriate API endpoint based on API tier.
   * @return Base URL for DeepL API
   */
    [[nodiscard]] std::string GetApiEndpoint() const;

    /**
   * Build the translation request URL with parameters.
   * @param text The text to translate
   * @return Full URL with encoded parameters
   */
    [[nodiscard]] std::string BuildRequestUrl(const std::string& text) const;

    /**
   * Parse the JSON response from DeepL API.
   * @param json The JSON response string
   * @return The translated text
   * @throws std::runtime_error if parsing fails
   */
    [[nodiscard]] static std::string ParseTranslationResponse(const std::string& json);

    /**
   * URL-encode a string for use in HTTP requests.
   * @param value The string to encode
   * @return URL-encoded string
   */
    [[nodiscard]] static std::string UrlEncode(const std::string& value);

    std::string m_ApiKey;
    bool m_UseFreeAPI;
    int m_TimeoutSeconds;
    std::string m_SourceLang;
    std::string m_TargetLang;
    std::string m_Formality;
  };

} // namespace Video2Card::Language::Translation
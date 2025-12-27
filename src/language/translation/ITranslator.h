#pragma once

#include <string>

namespace Video2Card::Language::Translation
{

  /**
 * Interface for Japanese to English translation.
 */
  class ITranslator
  {
public:

    virtual ~ITranslator() = default;

    /**
   * Translate Japanese text to English.
   * @param text The Japanese text to translate
   * @return The English translation, or empty if translation fails
   */
    [[nodiscard]] virtual std::string Translate(const std::string& text) = 0;

    /**
   * Check if translator is available.
   * @return true if translator is ready to use
   */
    [[nodiscard]] virtual bool IsAvailable() const = 0;
  };

} // namespace Video2Card::Language::Translation
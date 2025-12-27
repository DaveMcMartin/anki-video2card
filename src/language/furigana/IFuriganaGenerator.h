#pragma once

#include <string>

namespace Video2Card::Language::Furigana
{

  /**
 * Interface for generating furigana (reading annotations) for Japanese text.
 * Furigana is formatted in Anki style: kanji[reading]
 */
  class IFuriganaGenerator
  {
public:

    virtual ~IFuriganaGenerator() = default;

    /**
   * Generate furigana for Japanese text in Anki format.
   * @param text The Japanese text to add furigana to
   * @return Text with furigana in Anki format (e.g., "食[た]べる")
   */
    [[nodiscard]] virtual std::string Generate(const std::string& text) = 0;

    /**
   * Generate furigana for a single word in Anki format.
   * @param word The Japanese word
   * @return Word with furigana (e.g., "食[た]べる")
   */
    [[nodiscard]] virtual std::string GenerateForWord(const std::string& word) = 0;
  };

} // namespace Video2Card::Language::Furigana
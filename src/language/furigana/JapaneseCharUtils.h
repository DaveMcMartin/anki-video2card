#pragma once

#include <cctype>
#include <string>

namespace Video2Card::Language::Furigana
{

  /**
 * Utility functions for Japanese character manipulation.
 */
  class JapaneseCharUtils
  {
public:

    /**
   * Convert katakana string to hiragana.
   * @param katakana The katakana string to convert
   * @return The equivalent hiragana string
   */
    static std::string KatakanaToHiragana(const std::string& katakana);

    /**
   * Convert hiragana string to katakana.
   * @param hiragana The hiragana string to convert
   * @return The equivalent katakana string
   */
    static std::string HiraganaToKatakana(const std::string& hiragana);

    /**
   * Check if a character is kanji (CJK Unified Ideographs).
   * @param ch The character to check
   * @return true if the character is kanji
   */
    static bool IsKanji(const std::string& ch);

    /**
   * Check if a character is hiragana.
   * @param ch The character to check
   * @return true if the character is hiragana
   */
    static bool IsHiragana(const std::string& ch);

    /**
   * Check if a character is katakana.
   * @param ch The character to check
   * @return true if the character is katakana
   */
    static bool IsKatakana(const std::string& ch);

    /**
   * Check if a character is kana (hiragana or katakana).
   * @param ch The character to check
   * @return true if the character is kana
   */
    static bool IsKana(const std::string& ch);

    /**
   * Check if a string contains only kana (no kanji).
   * @param text The text to check
   * @return true if all characters are kana
   */
    static bool IsAllKana(const std::string& text);

private:

    JapaneseCharUtils() = delete;
    ~JapaneseCharUtils() = delete;
  };

} // namespace Video2Card::Language::Furigana
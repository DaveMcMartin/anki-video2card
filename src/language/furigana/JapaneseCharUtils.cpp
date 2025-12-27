#include "JapaneseCharUtils.h"

#include <algorithm>
#include <cstring>

namespace Video2Card::Language::Furigana
{

  // UTF-8 character classification helpers
  namespace
  {
    // Get the codepoint of a UTF-8 character sequence
    uint32_t GetUTF8Codepoint(const std::string& str, size_t& pos)
    {
      if (pos >= str.length()) {
        return 0;
      }

      unsigned char c = str[pos];

      // Single byte ASCII (0xxxxxxx)
      if ((c & 0x80) == 0) {
        pos++;
        return c;
      }

      // Two byte character (110xxxxx 10xxxxxx)
      if ((c & 0xE0) == 0xC0 && pos + 1 < str.length()) {
        uint32_t codepoint = ((c & 0x1F) << 6) | (str[pos + 1] & 0x3F);
        pos += 2;
        return codepoint;
      }

      // Three byte character (1110xxxx 10xxxxxx 10xxxxxx)
      if ((c & 0xF0) == 0xE0 && pos + 2 < str.length()) {
        uint32_t codepoint = ((c & 0x0F) << 12) | ((str[pos + 1] & 0x3F) << 6) | (str[pos + 2] & 0x3F);
        pos += 3;
        return codepoint;
      }

      // Four byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
      if ((c & 0xF8) == 0xF0 && pos + 3 < str.length()) {
        uint32_t codepoint =
            ((c & 0x07) << 18) | ((str[pos + 1] & 0x3F) << 12) | ((str[pos + 2] & 0x3F) << 6) | (str[pos + 3] & 0x3F);
        pos += 4;
        return codepoint;
      }

      pos++;
      return 0;
    }

    // Encode a codepoint to UTF-8 bytes
    std::string EncodeUTF8(uint32_t codepoint)
    {
      std::string result;

      if (codepoint <= 0x7F) {
        // ASCII
        result += static_cast<char>(codepoint);
      } else if (codepoint <= 0x7FF) {
        // Two bytes
        result += static_cast<char>(0xC0 | (codepoint >> 6));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
      } else if (codepoint <= 0xFFFF) {
        // Three bytes
        result += static_cast<char>(0xE0 | (codepoint >> 12));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
      } else if (codepoint <= 0x10FFFF) {
        // Four bytes
        result += static_cast<char>(0xF0 | (codepoint >> 18));
        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
      }

      return result;
    }
  } // namespace

  std::string JapaneseCharUtils::KatakanaToHiragana(const std::string& katakana)
  {
    std::string result;
    size_t pos = 0;

    while (pos < katakana.length()) {
      uint32_t codepoint = GetUTF8Codepoint(katakana, pos);

      // Katakana range: U+30A0 to U+30FF
      // Hiragana range: U+3040 to U+309F
      // The difference is 0x60
      if (codepoint >= 0x30A0 && codepoint <= 0x30FF) {
        codepoint -= 0x60;
      }

      result += EncodeUTF8(codepoint);
    }

    return result;
  }

  std::string JapaneseCharUtils::HiraganaToKatakana(const std::string& hiragana)
  {
    std::string result;
    size_t pos = 0;

    while (pos < hiragana.length()) {
      uint32_t codepoint = GetUTF8Codepoint(hiragana, pos);

      // Hiragana range: U+3040 to U+309F
      // Katakana range: U+30A0 to U+30FF
      // The difference is 0x60
      if (codepoint >= 0x3040 && codepoint <= 0x309F) {
        codepoint += 0x60;
      }

      result += EncodeUTF8(codepoint);
    }

    return result;
  }

  bool JapaneseCharUtils::IsKanji(const std::string& ch)
  {
    if (ch.empty()) {
      return false;
    }

    size_t pos = 0;
    uint32_t codepoint = GetUTF8Codepoint(ch, pos);

    // CJK Unified Ideographs: U+4E00 to U+9FFF
    // CJK Unified Ideographs Extension A: U+3400 to U+4DBF
    // CJK Unified Ideographs Extension B and beyond are less common
    return (codepoint >= 0x4E00 && codepoint <= 0x9FFF) || (codepoint >= 0x3400 && codepoint <= 0x4DBF);
  }

  bool JapaneseCharUtils::IsHiragana(const std::string& ch)
  {
    if (ch.empty()) {
      return false;
    }

    size_t pos = 0;
    uint32_t codepoint = GetUTF8Codepoint(ch, pos);

    // Hiragana: U+3040 to U+309F
    return codepoint >= 0x3040 && codepoint <= 0x309F;
  }

  bool JapaneseCharUtils::IsKatakana(const std::string& ch)
  {
    if (ch.empty()) {
      return false;
    }

    size_t pos = 0;
    uint32_t codepoint = GetUTF8Codepoint(ch, pos);

    // Katakana: U+30A0 to U+30FF
    return codepoint >= 0x30A0 && codepoint <= 0x30FF;
  }

  bool JapaneseCharUtils::IsKana(const std::string& ch)
  {
    return IsHiragana(ch) || IsKatakana(ch);
  }

  bool JapaneseCharUtils::IsAllKana(const std::string& text)
  {
    size_t pos = 0;

    while (pos < text.length()) {
      uint32_t codepoint = GetUTF8Codepoint(text, pos);

      // Check if codepoint is hiragana or katakana
      bool is_hiragana = (codepoint >= 0x3040 && codepoint <= 0x309F);
      bool is_katakana = (codepoint >= 0x30A0 && codepoint <= 0x30FF);

      if (!is_hiragana && !is_katakana) {
        // Allow spaces and punctuation
        if (codepoint == 0x0020 || // space
            codepoint == 0x3000 || // fullwidth space
            codepoint == 0x3002 || // fullwidth period
            codepoint == 0x3001 || // fullwidth comma
            codepoint == 0xFF01 || // fullwidth exclamation
            codepoint == 0xFF1F || // fullwidth question mark
            codepoint == 0x300C || // left corner bracket
            codepoint == 0x300D) { // right corner bracket
          continue;
        }
        return false;
      }
    }

    return true;
  }

} // namespace Video2Card::Language::Furigana
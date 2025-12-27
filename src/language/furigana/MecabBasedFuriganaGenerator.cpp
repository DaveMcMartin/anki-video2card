#include "MecabBasedFuriganaGenerator.h"

#include <stdexcept>

#include "JapaneseCharUtils.h"
#include "core/Logger.h"

namespace Video2Card::Language::Furigana
{

  MecabBasedFuriganaGenerator::MecabBasedFuriganaGenerator(std::shared_ptr<Morphology::IMorphologicalAnalyzer> analyzer)
      : m_Analyzer(analyzer)
  {
    if (!analyzer) {
      throw std::invalid_argument("Morphological analyzer cannot be null");
    }
  }

  std::string MecabBasedFuriganaGenerator::Generate(const std::string& text)
  {
    if (text.empty()) {
      return "";
    }

    try {
      auto tokens = m_Analyzer->Analyze(text);
      std::string result;

      for (const auto& token : tokens) {
        // For each token, generate furigana if it contains kanji
        // Otherwise, just append the surface form
        if (HasKanji(token.surface)) {
          result += FormatFuriganaAdvanced(token.surface, token.katakanaReading);
        } else {
          result += token.surface;
        }
      }

      return result;
    } catch (const std::exception& e) {
      AF_ERROR("Failed to generate furigana: {}", e.what());
      throw;
    }
  }

  std::string MecabBasedFuriganaGenerator::GenerateForWord(const std::string& word)
  {
    if (word.empty()) {
      return "";
    }

    try {
      auto tokens = m_Analyzer->Analyze(word);
      if (tokens.empty()) {
        return word;
      }

      return FormatFurigana(tokens[0].surface, tokens[0].katakanaReading);
    } catch (const std::exception& e) {
      AF_WARN("Failed to generate furigana for word '{}': {}", word, e.what());
      return word;
    }
  }

  std::string MecabBasedFuriganaGenerator::FormatFurigana(const std::string& word, const std::string& reading)
  {
    // If no reading provided, just return the word
    if (reading.empty() || reading == "*") {
      return word;
    }

    // If word is all kana (no kanji), just return the word
    if (!HasKanji(word)) {
      return word;
    }

    // Convert katakana reading to hiragana for Anki format
    std::string hiragana_reading = JapaneseCharUtils::KatakanaToHiragana(reading);

    // Return in Anki format: word[reading]
    return word + "[" + hiragana_reading + "]";
  }

  std::string MecabBasedFuriganaGenerator::FormatFuriganaAdvanced(const std::string& word, const std::string& reading)
  {
    // If no reading provided, just return the word
    if (reading.empty() || reading == "*") {
      return word;
    }

    // Convert katakana reading to hiragana
    std::string hiragana_reading = JapaneseCharUtils::KatakanaToHiragana(reading);

    // Simple approach: if the word contains kanji, apply furigana to the whole word
    // A more sophisticated approach would split kanji and kana within the word
    // For now, we'll use the simple approach that works well for most cases

    // Check if word starts or ends with kana that matches the reading
    // This handles cases like 食べる where we only want furigana on 食

    // For simplicity, apply furigana to the whole word if it contains any kanji
    return word + "[" + hiragana_reading + "]";
  }

  bool MecabBasedFuriganaGenerator::HasKanji(const std::string& text)
  {
    size_t pos = 0;
    while (pos < text.length()) {
      // Simple UTF-8 kanji detection
      // Kanji is in range U+4E00 to U+9FFF (mainly)
      unsigned char c = text[pos];

      if ((c & 0xF0) == 0xE0 && pos + 2 < text.length()) {
        // 3-byte UTF-8 character
        unsigned char b1 = text[pos];
        unsigned char b2 = text[pos + 1];
        unsigned char b3 = text[pos + 2];

        uint32_t codepoint = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);

        // Check if in kanji range
        if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF) || (codepoint >= 0x3400 && codepoint <= 0x4DBF)) {
          return true;
        }

        pos += 3;
      } else if ((c & 0x80) == 0) {
        // ASCII
        pos += 1;
      } else if ((c & 0xE0) == 0xC0) {
        // 2-byte
        pos += 2;
      } else if ((c & 0xF0) == 0xE0) {
        // 3-byte
        pos += 3;
      } else if ((c & 0xF8) == 0xF0) {
        // 4-byte
        pos += 4;
      } else {
        pos += 1;
      }
    }

    return false;
  }

} // namespace Video2Card::Language::Furigana
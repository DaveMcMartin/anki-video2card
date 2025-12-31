#include "MecabBasedFuriganaGenerator.h"

#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "JapaneseCharUtils.h"
#include "core/Logger.h"

namespace Video2Card::Language::Furigana
{

  namespace
  {

    struct UTF8Char
    {
      std::string str;
      size_t byteLength;
    };

    std::vector<UTF8Char> SplitUTF8(const std::string& text)
    {
      std::vector<UTF8Char> result;
      size_t pos = 0;

      while (pos < text.length()) {
        unsigned char c = text[pos];
        size_t len = 1;

        if ((c & 0x80) == 0) {
          len = 1;
        } else if ((c & 0xE0) == 0xC0) {
          len = 2;
        } else if ((c & 0xF0) == 0xE0) {
          len = 3;
        } else if ((c & 0xF8) == 0xF0) {
          len = 4;
        }

        if (pos + len <= text.length()) {
          result.push_back({text.substr(pos, len), len});
        }
        pos += len;
      }

      return result;
    }

    bool IsKanaChar(const std::string& ch)
    {
      return JapaneseCharUtils::IsKana(ch);
    }

    std::pair<size_t, size_t> FindKanjiBoundaries(const std::string& word)
    {
      auto chars = SplitUTF8(word);
      size_t lenKanaBefore = 0;
      size_t lenKanaAfter = 0;

      for (const auto& ch : chars) {
        if (!IsKanaChar(ch.str)) {
          break;
        }
        lenKanaBefore++;
      }

      for (auto it = chars.rbegin(); it != chars.rend(); ++it) {
        if (!IsKanaChar(it->str)) {
          break;
        }
        lenKanaAfter++;
      }

      return {lenKanaBefore, lenKanaAfter};
    }

    std::string SubstringByChars(const std::string& text, size_t startChar, size_t endChar)
    {
      auto chars = SplitUTF8(text);
      if (startChar >= chars.size()) {
        return "";
      }
      if (endChar > chars.size()) {
        endChar = chars.size();
      }

      std::string result;
      for (size_t i = startChar; i < endChar; i++) {
        result += chars[i].str;
      }
      return result;
    }

    std::string SubstringByCharsFromEnd(const std::string& text, size_t count)
    {
      auto chars = SplitUTF8(text);
      if (count >= chars.size()) {
        return text;
      }
      return SubstringByChars(text, chars.size() - count, chars.size());
    }

    size_t GetCharCount(const std::string& text)
    {
      return SplitUTF8(text).size();
    }

    struct Dismembered
    {
      std::string word;
      std::string reading;
      std::string tail;

      std::string Assemble() const { return word + "[" + reading + "]" + tail; }
    };

    struct CompoundSplit
    {
      Dismembered first;
      Dismembered second;
    };

    std::optional<Dismembered> DismemberExpression(const std::string& expr)
    {
      size_t furiganaStart = expr.find('[');
      if (furiganaStart == std::string::npos || furiganaStart < 1) {
        return std::nullopt;
      }

      size_t furiganaEnd = expr.find(']');
      if (furiganaEnd == std::string::npos || furiganaEnd < 3) {
        return std::nullopt;
      }

      return Dismembered{expr.substr(0, furiganaStart),
                         expr.substr(furiganaStart + 1, furiganaEnd - furiganaStart - 1),
                         expr.substr(furiganaEnd + 1)};
    }

    size_t FindCommonPrefixLength(const std::string& stem, const std::string& reading)
    {
      auto stemChars = SplitUTF8(stem);
      auto readingChars = SplitUTF8(reading);

      size_t commonLen = 0;
      size_t minLen = std::min(stemChars.size(), readingChars.size());

      for (size_t i = 0; i < minLen; i++) {
        if (stemChars[i].str != readingChars[i].str) {
          break;
        }
        commonLen++;
      }

      return commonLen;
    }

    std::optional<CompoundSplit> FindCommonKana(const Dismembered& expr)
    {
      auto wordChars = SplitUTF8(expr.word);
      auto readingChars = SplitUTF8(expr.reading);

      size_t startIndex = std::max<size_t>(1, FindCommonPrefixLength(expr.word, expr.reading));

      for (size_t wordIdx = startIndex; wordIdx < wordChars.size(); wordIdx++) {
        for (size_t readingIdx = startIndex; readingIdx < readingChars.size(); readingIdx++) {
          if (wordChars[wordIdx].str == readingChars[readingIdx].str) {
            std::string wordSuffix = SubstringByChars(expr.word, wordIdx, wordChars.size());
            std::string readingSuffix = SubstringByChars(expr.reading, readingIdx, readingChars.size());

            size_t prefixLen = FindCommonPrefixLength(wordSuffix, readingSuffix);

            if (wordIdx > readingIdx) {
              continue;
            }

            return CompoundSplit{
                Dismembered{SubstringByChars(expr.word, 0, wordIdx),
                            SubstringByChars(expr.reading, 0, readingIdx),
                            SubstringByChars(expr.reading, readingIdx, readingIdx + prefixLen)},
                Dismembered{SubstringByChars(expr.word, wordIdx + prefixLen, wordChars.size()),
                            SubstringByChars(expr.reading, readingIdx + prefixLen, readingChars.size()),
                            expr.tail}};
          }
        }
      }

      return std::nullopt;
    }

    std::string BreakCompoundFuriganaChunk(const std::string& expr);

    std::string BreakCompoundFuriganaChunk(const std::string& expr)
    {
      auto dismembered = DismemberExpression(expr);
      if (!dismembered) {
        return expr;
      }

      auto compound = FindCommonKana(*dismembered);
      if (!compound) {
        return expr;
      }

      return compound->first.Assemble() + " " + BreakCompoundFuriganaChunk(compound->second.Assemble());
    }

    std::string BreakCompoundFurigana(const std::string& expr)
    {
      if (expr.empty()) {
        return expr;
      }

      size_t leadingSpaces = 0;
      while (leadingSpaces < expr.length() && expr[leadingSpaces] == ' ') {
        leadingSpaces++;
      }

      size_t trailingSpaces = 0;
      size_t pos = expr.length();
      while (pos > leadingSpaces && expr[pos - 1] == ' ') {
        trailingSpaces++;
        pos--;
      }

      std::string trimmed = expr.substr(leadingSpaces, expr.length() - leadingSpaces - trailingSpaces);
      if (trimmed.empty()) {
        return expr;
      }

      std::vector<std::string> chunks;
      std::string current;

      for (char ch : trimmed) {
        if (ch == ' ') {
          if (!current.empty()) {
            chunks.push_back(current);
            current.clear();
          }
        } else {
          current += ch;
        }
      }

      if (!current.empty()) {
        chunks.push_back(current);
      }

      std::string result;
      for (size_t i = 0; i < leadingSpaces; i++) {
        result += ' ';
      }

      for (size_t i = 0; i < chunks.size(); i++) {
        if (i > 0) {
          result += " ";
        }
        result += BreakCompoundFuriganaChunk(chunks[i]);
      }

      for (size_t i = 0; i < trailingSpaces; i++) {
        result += ' ';
      }

      return result;
    }

    std::string FormatOutputInternal(const std::string& kanji, const std::string& reading)
    {
      auto [nBefore, nAfter] = FindKanjiBoundaries(kanji);
      std::string outExpr;

      size_t kanjiCharCount = GetCharCount(kanji);
      size_t readingCharCount = GetCharCount(reading);

      if (nBefore == 0) {
        if (nAfter == 0) {
          outExpr = " " + kanji + "[" + reading + "]";
        } else {
          std::string kanjiCore = SubstringByChars(kanji, 0, kanjiCharCount - nAfter);
          std::string readingCore = SubstringByChars(reading, 0, readingCharCount - nAfter);
          std::string suffix = SubstringByCharsFromEnd(kanji, nAfter);
          outExpr = " " + kanjiCore + "[" + readingCore + "]" + suffix;
        }
      } else {
        if (nAfter == 0) {
          std::string prefix = SubstringByChars(kanji, 0, nBefore);
          std::string kanjiCore = SubstringByChars(kanji, nBefore, kanjiCharCount);
          std::string readingCore = SubstringByChars(reading, nBefore, readingCharCount);
          outExpr = prefix + " " + kanjiCore + "[" + readingCore + "]";
        } else {
          std::string prefix = SubstringByChars(kanji, 0, nBefore);
          std::string kanjiCore = SubstringByChars(kanji, nBefore, kanjiCharCount - nAfter);
          std::string readingCore = SubstringByChars(reading, nBefore, readingCharCount - nAfter);
          std::string suffix = SubstringByCharsFromEnd(kanji, nAfter);
          outExpr = prefix + " " + kanjiCore + "[" + readingCore + "]" + suffix;
        }
      }

      return BreakCompoundFurigana(outExpr);
    }

  } // namespace

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

      AF_DEBUG("Furigana generation for text: '{}'", text);
      AF_DEBUG("MeCab returned {} tokens", tokens.size());

      for (const auto& token : tokens) {
        AF_DEBUG("Token: surface='{}', reading='{}', hasKanji={}",
                 token.surface,
                 token.katakanaReading,
                 HasKanji(token.surface));

        if (HasKanji(token.surface)) {
          std::string formatted = FormatFuriganaAdvanced(token.surface, token.katakanaReading);
          AF_DEBUG("  Formatted as: '{}'", formatted);
          result += formatted;
        } else {
          result += token.surface;
        }
      }

      AF_DEBUG("Before trimming: '{}'", result);

      while (!result.empty() && result.front() == ' ') {
        result.erase(0, 1);
      }
      while (!result.empty() && result.back() == ' ') {
        result.pop_back();
      }
      AF_DEBUG("After trimming: '{}'", result);
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
    if (!HasKanji(word)) {
      return word;
    }

    if (reading.empty() || reading == "*") {
      return " " + word;
    }

    std::string hiraganaReading = JapaneseCharUtils::KatakanaToHiragana(reading);
    return FormatOutputInternal(word, hiraganaReading);
  }

  std::string MecabBasedFuriganaGenerator::FormatFuriganaAdvanced(const std::string& word, const std::string& reading)
  {
    if (!HasKanji(word)) {
      return word;
    }

    if (reading.empty() || reading == "*") {
      return " " + word;
    }

    std::string hiraganaReading = JapaneseCharUtils::KatakanaToHiragana(reading);
    return FormatOutputInternal(word, hiraganaReading);
  }

  bool MecabBasedFuriganaGenerator::HasKanji(const std::string& text)
  {
    size_t pos = 0;
    while (pos < text.length()) {
      unsigned char c = text[pos];

      if ((c & 0xF0) == 0xE0 && pos + 2 < text.length()) {
        unsigned char b1 = text[pos];
        unsigned char b2 = text[pos + 1];
        unsigned char b3 = text[pos + 2];

        uint32_t codepoint = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);

        if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF) || (codepoint >= 0x3400 && codepoint <= 0x4DBF)) {
          return true;
        }

        pos += 3;
      } else if ((c & 0x80) == 0) {
        pos += 1;
      } else if ((c & 0xE0) == 0xC0) {
        pos += 2;
      } else if ((c & 0xF0) == 0xE0) {
        pos += 3;
      } else if ((c & 0xF8) == 0xF0) {
        pos += 4;
      } else {
        pos += 1;
      }
    }

    return false;
  }

} // namespace Video2Card::Language::Furigana
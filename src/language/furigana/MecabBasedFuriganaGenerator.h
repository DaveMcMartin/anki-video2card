#pragma once

#include <memory>
#include <string>

#include "IFuriganaGenerator.h"
#include "language/morphology/IMorphologicalAnalyzer.h"

namespace Video2Card::Language::Furigana
{

  /**
 * Furigana generator using Mecab morphological analysis.
 * Breaks Japanese text into morphemes and adds furigana readings
 * in Anki format.
 */
  class MecabBasedFuriganaGenerator : public IFuriganaGenerator
  {
public:

    /**
   * Create a furigana generator with Mecab morphological analyzer.
   * @param analyzer The morphological analyzer to use for breaking text into words
   * @throws std::invalid_argument if analyzer is null
   */
    explicit MecabBasedFuriganaGenerator(std::shared_ptr<Morphology::IMorphologicalAnalyzer> analyzer);

    ~MecabBasedFuriganaGenerator() override = default;

    /**
   * Generate furigana for Japanese text in Anki format.
   * @param text The Japanese text to annotate
   * @return Text with furigana in Anki format (e.g., "食[た]べる")
   * @throws std::runtime_error if analysis fails
   */
    [[nodiscard]] std::string Generate(const std::string& text) override;

    /**
   * Generate furigana for a single word in Anki format.
   * @param word The Japanese word to annotate
   * @return Word with furigana (e.g., "食[た]べる")
   */
    [[nodiscard]] std::string GenerateForWord(const std::string& word) override;

private:

    std::shared_ptr<Morphology::IMorphologicalAnalyzer> m_Analyzer;

    /**
   * Format a word and its reading into Anki furigana format.
   * If word contains no kanji, returns just the word.
   * If word is all kana, returns just the word.
   * Otherwise, returns word[reading] where reading is hiragana.
   * @param word The surface form of the word
   * @param reading The katakana reading
   * @return Formatted furigana string
   */
    std::string FormatFurigana(const std::string& word, const std::string& reading);

    /**
   * Advanced furigana formatting that handles word boundaries better.
   * Similar to FormatFurigana but with improved handling of mixed kana-kanji words.
   * @param word The surface form of the word
   * @param reading The katakana reading
   * @return Formatted furigana string
   */
    std::string FormatFuriganaAdvanced(const std::string& word, const std::string& reading);

    /**
   * Check if a string contains any kanji characters.
   * @param text The text to check
   * @return true if the text contains kanji
   */
    static bool HasKanji(const std::string& text);
  };

} // namespace Video2Card::Language::Furigana
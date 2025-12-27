#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace Video2Card::Language::Analyzer
{

  /**
 * Interface for local Japanese sentence analysis.
 * Provides sentence parsing, word definitions, and furigana generation
 * without relying on external AI services.
 */
  class ILocalAnalyzer
  {
public:

    virtual ~ILocalAnalyzer() = default;

    /**
   * Analyze a Japanese sentence and return structured results.
   * @param sentence The Japanese sentence to analyze
   * @param targetWord Optional target word to focus on (if empty, most important word is selected)
   * @return JSON object with analysis results including:
   *         - sentence: The original sentence with target word highlighted
   *         - translation: English translation (if available)
   *         - target_word: The dictionary form of the target word
   *         - target_word_furigana: Target word with furigana
   *         - furigana: Full sentence with furigana
   *         - definition: Definition of the target word
   *         - pitch_accent: Pitch accent pattern (if available)
   */
    [[nodiscard]] virtual nlohmann::json AnalyzeSentence(const std::string& sentence,
                                                         const std::string& targetWord = "") = 0;

    /**
   * Check if the analyzer is ready to use (all components initialized).
   * @return true if the analyzer can process text
   */
    [[nodiscard]] virtual bool IsReady() const = 0;

    /**
   * Get human-readable name of this analyzer.
   * @return Analyzer name
   */
    [[nodiscard]] virtual std::string GetName() const = 0;
  };

} // namespace Video2Card::Language::Analyzer
#pragma once

#include <string>

#include "MecabToken.h"

namespace Video2Card::Language::Morphology
{

  /**
 * Interface for morphological analysis of Japanese text.
 * Implementations use tools like Mecab to break down sentences
 * and extract grammatical information.
 */
  class IMorphologicalAnalyzer
  {
public:

    virtual ~IMorphologicalAnalyzer() = default;

    /**
   * Analyze Japanese text and return tokens with morphological information.
   * @param text The Japanese text to analyze
   * @return List of MecabToken objects with morphological data
   * @throws std::runtime_error if analysis fails
   */
    [[nodiscard]] virtual MecabTokenList Analyze(const std::string& text) = 0;

    /**
   * Get the dictionary form (headword) of a word.
   * @param surface The surface form of the word
   * @return The dictionary form, or empty string if not found
   */
    [[nodiscard]] virtual std::string GetDictionaryForm(const std::string& surface) = 0;

    /**
   * Get the reading of a word in katakana.
   * @param surface The surface form of the word
   * @return The katakana reading, or empty string if not found
   */
    [[nodiscard]] virtual std::string GetReading(const std::string& surface) = 0;
  };

} // namespace Video2Card::Language::Morphology
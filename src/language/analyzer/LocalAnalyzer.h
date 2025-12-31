#pragma once

#include <memory>
#include <string>

#include "ILocalAnalyzer.h"
#include "language/dictionary/IDictionaryClient.h"
#include "language/furigana/IFuriganaGenerator.h"
#include "language/morphology/IMorphologicalAnalyzer.h"
#include "language/pitch_accent/IPitchAccentLookup.h"
#include "language/translation/ITranslator.h"

namespace Video2Card::Language::Analyzer
{

  using Dictionary::IDictionaryClient;
  using Furigana::IFuriganaGenerator;
  using Morphology::IMorphologicalAnalyzer;
  using PitchAccent::IPitchAccentLookup;
  using Translation::ITranslator;

  /**
 * Local Japanese sentence analyzer without external AI.
 * Combines Mecab, furigana generation, and dictionary lookups
 * to provide complete sentence analysis.
 */
  class LocalAnalyzer : public ILocalAnalyzer
  {
public:

    /**
   * Create a local analyzer with all required components.
   * @param morphAnalyzer Morphological analyzer (e.g., Mecab)
   * @param furiganaGen Furigana generator
   * @param dictClient Dictionary client for word definitions
   * @param translator Optional translator for sentence translation
   */
    LocalAnalyzer(std::shared_ptr<IMorphologicalAnalyzer> morphAnalyzer,
                  std::shared_ptr<IFuriganaGenerator> furiganaGen,
                  std::shared_ptr<IDictionaryClient> dictClient,
                  std::shared_ptr<ITranslator> translator = nullptr,
                  std::shared_ptr<IPitchAccentLookup> pitchAccent = nullptr);

    ~LocalAnalyzer() override = default;

    /**
   * Analyze a Japanese sentence locally.
   * @param sentence The sentence to analyze
   * @param targetWord Optional target word to focus on
   * @return JSON with analysis results
   */
    [[nodiscard]] nlohmann::json AnalyzeSentence(const std::string& sentence,
                                                 const std::string& targetWord = "") override;

    /**
   * Check if all components are initialized and ready.
   * @return true if the analyzer is ready to use
   */
    [[nodiscard]] bool IsReady() const override;

    /**
   * Get the analyzer name.
   * @return "Local Analyzer"
   */
    [[nodiscard]] std::string GetName() const override;

private:

    std::shared_ptr<IMorphologicalAnalyzer> m_MorphAnalyzer;
    std::shared_ptr<IFuriganaGenerator> m_FuriganaGen;
    std::shared_ptr<IDictionaryClient> m_DictClient;
    std::shared_ptr<ITranslator> m_Translator;
    std::shared_ptr<IPitchAccentLookup> m_PitchAccent;

    /**
   * Select the target word if not provided.
   * Uses morphological analysis to find the most important word.
   * @param sentence The sentence to analyze
   * @return The selected target word
   */
    [[nodiscard]] std::string SelectTargetWord(const std::string& sentence);

    /**
   * Get the dictionary form (headword) of a word.
   * @param surface The surface form
   * @return The dictionary form
   */
    [[nodiscard]] std::string GetDictionaryForm(const std::string& surface);

    /**
   * Get the katakana reading for a word.
   * @param surface The surface form
   * @return The katakana reading
   */
    [[nodiscard]] std::string GetReading(const std::string& surface);
  };

} // namespace Video2Card::Language::Analyzer
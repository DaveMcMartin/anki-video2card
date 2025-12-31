#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Video2Card::Language
{
  class ILanguage;
}

namespace Video2Card::Language::Services
{
  class ILanguageService;
}

namespace Video2Card::Language::Morphology
{
  class IMorphologicalAnalyzer;
}

namespace Video2Card::Language::Furigana
{
  class IFuriganaGenerator;
}

namespace Video2Card::Language::Dictionary
{
  class IDictionaryClient;
}

namespace Video2Card::Language::Translation
{
  class ITranslator;
}

namespace Video2Card::Language::PitchAccent
{
  class IPitchAccentLookup;
}

namespace Video2Card::Language::Analyzer
{

  /**
 * Unified sentence analyzer that uses language services.
 * Combines MeCab, furigana, dictionary, and translation services
 * to provide complete sentence analysis.
 */
  class SentenceAnalyzer
  {
public:

    SentenceAnalyzer();
    ~SentenceAnalyzer() = default;

    /**
   * Set the language services to use for analysis.
   * @param services Vector of available language services
   */
    void SetLanguageServices(const std::vector<std::unique_ptr<Services::ILanguageService>>* services);

    /**
   * Initialize the analyzer with MeCab and other components.
   * @param basePath Base path for assets (database, etc.)
   * @return true if initialization succeeded
   */
    bool Initialize(const std::string& basePath = "");

    /**
   * Analyze a Japanese sentence.
   * @param sentence The sentence to analyze
   * @param targetWord Optional target word to focus on
   * @param language The language configuration (unused for now)
   * @return JSON with analysis results
   */
    [[nodiscard]] nlohmann::json
    AnalyzeSentence(const std::string& sentence, const std::string& targetWord, ILanguage* language = nullptr);

    /**
   * Check if the analyzer is ready to use.
   * @return true if all required components are initialized
   */
    [[nodiscard]] bool IsReady() const;

private:

    /**
   * Get the translator from language services.
   * @return Translator instance or nullptr
   */
    [[nodiscard]] std::shared_ptr<Translation::ITranslator> GetTranslator() const;

    /**
   * Select the target word if not provided.
   * @param sentence The sentence to analyze
   * @return The selected target word
   */
    [[nodiscard]] std::string SelectTargetWord(const std::string& sentence);

    /**
   * Get the dictionary form of a word.
   * @param surface The surface form
   * @return Dictionary form
   */
    [[nodiscard]] std::string GetDictionaryForm(const std::string& surface);

    /**
   * Get the reading of a word.
   * @param surface The surface form
   * @return Katakana reading
   */
    [[nodiscard]] std::string GetReading(const std::string& surface);

    const std::vector<std::unique_ptr<Services::ILanguageService>>* m_LanguageServices;
    std::shared_ptr<Morphology::IMorphologicalAnalyzer> m_MorphAnalyzer;
    std::shared_ptr<Furigana::IFuriganaGenerator> m_FuriganaGen;
    std::shared_ptr<Dictionary::IDictionaryClient> m_DictClient;
    std::shared_ptr<PitchAccent::IPitchAccentLookup> m_PitchAccent;
  };

} // namespace Video2Card::Language::Analyzer
#include "LocalAnalyzer.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "core/Logger.h"
#include "language/furigana/JapaneseCharUtils.h"
#include "language/morphology/MecabToken.h"

namespace Video2Card::Language::Analyzer
{

  LocalAnalyzer::LocalAnalyzer(std::shared_ptr<IMorphologicalAnalyzer> morphAnalyzer,
                               std::shared_ptr<IFuriganaGenerator> furiganaGen,
                               std::shared_ptr<IDictionaryClient> dictClient,
                               std::shared_ptr<ITranslator> translator,
                               std::shared_ptr<IPitchAccentLookup> pitchAccent)
      : m_MorphAnalyzer(std::move(morphAnalyzer))
      , m_FuriganaGen(std::move(furiganaGen))
      , m_DictClient(std::move(dictClient))
      , m_Translator(std::move(translator))
      , m_PitchAccent(std::move(pitchAccent))
  {
    if (!m_MorphAnalyzer) {
      throw std::invalid_argument("Morphological analyzer is required");
    }
    AF_INFO("LocalAnalyzer initialized successfully");
  }

  nlohmann::json LocalAnalyzer::AnalyzeSentence(const std::string& sentence, const std::string& targetWord)
  {
    nlohmann::json result;

    if (sentence.empty()) {
      result["error"] = "Sentence cannot be empty";
      return result;
    }

    try {
      // Determine the target word
      std::string focusWord = targetWord.empty() ? SelectTargetWord(sentence) : targetWord;

      if (focusWord.empty()) {
        AF_WARN("Could not determine target word for sentence: {}", sentence);
        focusWord = "詞"; // Fallback
      }

      // Generate furigana for the sentence if available
      std::string sentenceWithFurigana = sentence;
      if (m_FuriganaGen) {
        sentenceWithFurigana = m_FuriganaGen->Generate(sentence);
      }

      // Get the dictionary form and reading of the target word
      std::string dictionaryForm = GetDictionaryForm(focusWord);
      std::string reading = GetReading(focusWord);

      // Generate furigana for the target word
      std::string targetWordFurigana;
      if (m_FuriganaGen && !reading.empty()) {
        targetWordFurigana = m_FuriganaGen->GenerateForWord(focusWord);
      } else {
        targetWordFurigana = focusWord;
      }

      // Look up the definition if dictionary is available
      std::string definition;
      if (m_DictClient) {
        auto dictEntry = m_DictClient->LookupWord(focusWord, dictionaryForm);
        definition = dictEntry.definition;
      }

      // Translate the sentence if translator is available
      std::string translation;
      if (m_Translator) {
        try {
          translation = m_Translator->Translate(sentence);
        } catch (const std::exception& e) {
          AF_WARN("Translation failed: {}", e.what());
        }
      }

      std::string pitchAccent;
      if (m_PitchAccent) {
        std::string lookupWord = dictionaryForm.empty() ? focusWord : dictionaryForm;
        auto pitchEntries = m_PitchAccent->LookupWord(lookupWord, reading);
        if (pitchEntries.empty() && !reading.empty()) {
          pitchEntries = m_PitchAccent->LookupWord(reading, reading);
        }
        pitchAccent = m_PitchAccent->FormatAsHtml(pitchEntries);
      }

      result["sentence"] = sentence;
      result["translation"] = translation;
      result["target_word"] = dictionaryForm.empty() ? focusWord : dictionaryForm;
      result["target_word_furigana"] = targetWordFurigana;
      result["furigana"] = sentenceWithFurigana;
      result["definition"] = definition;
      result["pitch_accent"] = pitchAccent;

      AF_DEBUG("Analysis complete for sentence: {}", sentence);

      return result;
    } catch (const std::exception& e) {
      AF_ERROR("Failed to analyze sentence: {}", e.what());
      result["error"] = std::string("Analysis failed: ") + e.what();
      return result;
    }
  }

  bool LocalAnalyzer::IsReady() const
  {
    return m_MorphAnalyzer != nullptr;
  }

  std::string LocalAnalyzer::GetName() const
  {
    return "Local Analyzer (Mecab + Dictionary)";
  }

  std::string LocalAnalyzer::SelectTargetWord(const std::string& sentence)
  {
    try {
      auto tokens = m_MorphAnalyzer->Analyze(sentence);

      // Find the first content word
      for (const auto& token : tokens) {
        if (!token.surface.empty() &&
            (token.partOfSpeech == "noun" || token.partOfSpeech == "verb" || token.partOfSpeech == "adjective"))
        {
          return token.surface;
        }
      }

      // If no content word found, return the first non-empty token
      for (const auto& token : tokens) {
        if (!token.surface.empty()) {
          return token.surface;
        }
      }
    } catch (const std::exception& e) {
      AF_WARN("Failed to select target word: {}", e.what());
    }

    return "";
  }

  std::string LocalAnalyzer::GetDictionaryForm(const std::string& surface)
  {
    try {
      return m_MorphAnalyzer->GetDictionaryForm(surface);
    } catch (const std::exception& e) {
      AF_WARN("Failed to get dictionary form: {}", e.what());
      return surface;
    }
  }

  std::string LocalAnalyzer::GetReading(const std::string& surface)
  {
    try {
      return m_MorphAnalyzer->GetReading(surface);
    } catch (const std::exception& e) {
      AF_WARN("Failed to get reading: {}", e.what());
      return "";
    }
  }

} // namespace Video2Card::Language::Analyzer
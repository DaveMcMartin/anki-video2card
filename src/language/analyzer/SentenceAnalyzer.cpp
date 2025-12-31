#include "SentenceAnalyzer.h"

#include <stdexcept>

#include "core/Logger.h"
#include "language/ILanguage.h"
#include "language/dictionary/JMDictionary.h"
#include "language/furigana/MecabBasedFuriganaGenerator.h"
#include "language/morphology/MecabAnalyzer.h"
#include "language/services/DeepLService.h"
#include "language/services/ILanguageService.h"

namespace Video2Card::Language::Analyzer
{

  SentenceAnalyzer::SentenceAnalyzer()
      : m_LanguageServices(nullptr)
      , m_MorphAnalyzer(nullptr)
      , m_FuriganaGen(nullptr)
      , m_DictClient(nullptr)
  {}

  void SentenceAnalyzer::SetLanguageServices(const std::vector<std::unique_ptr<Services::ILanguageService>>* services)
  {
    m_LanguageServices = services;
  }

  bool SentenceAnalyzer::Initialize(const std::string& basePath)
  {
    try {
      // Initialize MeCab analyzer
      m_MorphAnalyzer = std::make_shared<Morphology::MecabAnalyzer>();
      AF_INFO("MeCab analyzer initialized");

      // Initialize furigana generator
      m_FuriganaGen = std::make_shared<Furigana::MecabBasedFuriganaGenerator>(m_MorphAnalyzer);
      AF_INFO("Furigana generator initialized");

      // Initialize dictionary client
      try {
        std::string dbPath = basePath + "assets/jmdict.db";
        m_DictClient = std::make_shared<Dictionary::JMDictionary>(dbPath);
        AF_INFO("Dictionary client initialized");
      } catch (const std::exception& e) {
        AF_WARN("Failed to initialize dictionary client: {}", e.what());
        m_DictClient = nullptr;
      }

      return true;

    } catch (const std::exception& e) {
      AF_ERROR("Failed to initialize SentenceAnalyzer: {}", e.what());
      return false;
    }
  }

  nlohmann::json
  SentenceAnalyzer::AnalyzeSentence(const std::string& sentence, const std::string& targetWord, ILanguage* language)
  {
    (void) language; // Not currently used

    nlohmann::json result;

    if (sentence.empty()) {
      result["error"] = "Sentence cannot be empty";
      return result;
    }

    if (!IsReady()) {
      result["error"] = "Analyzer not initialized";
      return result;
    }

    try {
      // Determine the target word
      std::string focusWord = targetWord.empty() ? SelectTargetWord(sentence) : targetWord;

      if (focusWord.empty()) {
        AF_WARN("Could not determine target word for sentence: {}", sentence);
        focusWord = "詞"; // Fallback
      }

      // Generate furigana for the sentence
      std::string sentenceWithFurigana = sentence;
      if (m_FuriganaGen) {
        try {
          sentenceWithFurigana = m_FuriganaGen->Generate(sentence);
        } catch (const std::exception& e) {
          AF_WARN("Failed to generate furigana: {}", e.what());
        }
      }

      // Get the dictionary form and reading of the target word
      std::string dictionaryForm = GetDictionaryForm(focusWord);
      std::string reading = GetReading(focusWord);

      // Generate furigana for the target word
      std::string targetWordFurigana;
      if (m_FuriganaGen && !reading.empty()) {
        try {
          targetWordFurigana = m_FuriganaGen->GenerateForWord(focusWord);
        } catch (const std::exception& e) {
          AF_WARN("Failed to generate target word furigana: {}", e.what());
          targetWordFurigana = focusWord;
        }
      } else {
        targetWordFurigana = focusWord;
      }

      // Look up the definition
      std::string definition;
      if (m_DictClient) {
        try {
          auto dictEntry = m_DictClient->LookupWord(focusWord, dictionaryForm);
          definition = dictEntry.definition;
        } catch (const std::exception& e) {
          AF_WARN("Failed to lookup definition: {}", e.what());
        }
      }

      // Translate the sentence using language services
      std::string translation;
      auto translator = GetTranslator();
      if (translator) {
        try {
          translation = translator->Translate(sentence);
        } catch (const std::exception& e) {
          AF_WARN("Translation failed: {}", e.what());
        }
      }

      // Build the result JSON
      result["sentence"] = sentence;
      result["translation"] = translation;
      result["target_word"] = dictionaryForm.empty() ? focusWord : dictionaryForm;
      result["target_word_furigana"] = targetWordFurigana;
      result["furigana"] = sentenceWithFurigana;
      result["definition"] = definition;
      result["pitch_accent"] = ""; // Not implemented yet

      AF_DEBUG("Analysis complete for sentence: {}", sentence);

      return result;

    } catch (const std::exception& e) {
      AF_ERROR("Failed to analyze sentence: {}", e.what());
      result["error"] = std::string("Analysis failed: ") + e.what();
      return result;
    }
  }

  bool SentenceAnalyzer::IsReady() const
  {
    return m_MorphAnalyzer && m_FuriganaGen;
  }

  std::shared_ptr<Translation::ITranslator> SentenceAnalyzer::GetTranslator() const
  {
    if (!m_LanguageServices) {
      return nullptr;
    }

    for (const auto& service : *m_LanguageServices) {
      if (service->GetType() == "translator" && service->IsAvailable()) {
        // Try to cast to DeepLService to get the translator
        if (service->GetId() == "deepl") {
          auto* deeplService = dynamic_cast<Services::DeepLService*>(service.get());
          if (deeplService) {
            return deeplService->GetTranslator();
          }
        }
      }
    }

    return nullptr;
  }

  std::string SentenceAnalyzer::SelectTargetWord(const std::string& sentence)
  {
    if (!m_MorphAnalyzer) {
      return "";
    }

    try {
      auto tokens = m_MorphAnalyzer->Analyze(sentence);

      // Find the first content word (noun, verb, or adjective)
      for (const auto& token : tokens) {
        if (!token.surface.empty() &&
            (token.partOfSpeech == "名詞" || token.partOfSpeech == "動詞" || token.partOfSpeech == "形容詞"))
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

  std::string SentenceAnalyzer::GetDictionaryForm(const std::string& surface)
  {
    if (!m_MorphAnalyzer) {
      return surface;
    }

    try {
      return m_MorphAnalyzer->GetDictionaryForm(surface);
    } catch (const std::exception& e) {
      AF_WARN("Failed to get dictionary form: {}", e.what());
      return surface;
    }
  }

  std::string SentenceAnalyzer::GetReading(const std::string& surface)
  {
    if (!m_MorphAnalyzer) {
      return "";
    }

    try {
      return m_MorphAnalyzer->GetReading(surface);
    } catch (const std::exception& e) {
      AF_WARN("Failed to get reading: {}", e.what());
      return "";
    }
  }

} // namespace Video2Card::Language::Analyzer
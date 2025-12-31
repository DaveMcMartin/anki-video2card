#include "CTranslate2Translator.h"

#include <algorithm>
#include <codecvt>
#include <ctranslate2/translator.h>
#include <locale>
#include <sstream>
#include <stdexcept>

#include "core/Logger.h"

namespace Video2Card::Language::Translation
{

  CTranslate2Translator::CTranslate2Translator(const std::string& modelPath)
      : m_Device("cpu")
      , m_BeamSize(4)
      , m_IsAvailable(false)
  {
    try {
      ctranslate2::Device device = ctranslate2::Device::CPU;
      m_Translator = std::make_unique<ctranslate2::Translator>(modelPath, device);
      m_IsAvailable = true;
      AF_INFO("CTranslate2 translator initialized successfully from: {}", modelPath);
    } catch (const std::exception& e) {
      AF_ERROR("Failed to initialize CTranslate2 translator: {}", e.what());
      m_IsAvailable = false;
    }
  }

  CTranslate2Translator::~CTranslate2Translator() = default;

  std::string CTranslate2Translator::Translate(const std::string& text)
  {
    if (text.empty()) {
      AF_DEBUG("CTranslate2: Empty input text");
      return "";
    }

    if (!m_IsAvailable || !m_Translator) {
      AF_WARN("CTranslate2 translator not available - returning original text");
      return text;
    }

    try {
      AF_DEBUG("CTranslate2: Starting translation for: '{}'", text);

      std::vector<std::string> tokens = Tokenize(text);

      AF_DEBUG("CTranslate2: Tokenized into {} tokens", tokens.size());
      std::string tokensStr;
      for (size_t i = 0; i < std::min(tokens.size(), size_t(10)); i++) {
        tokensStr += "[" + tokens[i] + "] ";
      }
      if (tokens.size() > 10) {
        tokensStr += "...";
      }
      AF_DEBUG("CTranslate2: First tokens: {}", tokensStr);

      if (tokens.empty()) {
        AF_WARN("CTranslate2: Tokenization produced no tokens");
        return text;
      }

      std::vector<std::vector<std::string>> sourceBatch = {tokens};

      ctranslate2::TranslationOptions options;
      options.beam_size = m_BeamSize;
      options.max_decoding_length = 100;
      options.length_penalty = 1.0;
      options.repetition_penalty = 1.2;
      options.no_repeat_ngram_size = 3;

      AF_DEBUG("CTranslate2: Calling translate_batch with beam_size={}", m_BeamSize);
      auto results = m_Translator->translate_batch(sourceBatch, options);

      if (results.empty()) {
        AF_ERROR("CTranslate2: translate_batch returned empty results");
        return text;
      }

      if (results[0].hypotheses.empty()) {
        AF_ERROR("CTranslate2: First result has no hypotheses");
        return text;
      }

      AF_DEBUG("CTranslate2: Got {} hypotheses", results[0].hypotheses.size());
      AF_DEBUG("CTranslate2: First hypothesis has {} tokens", results[0].hypotheses[0].size());

      std::string hypothesisStr;
      for (size_t i = 0; i < std::min(results[0].hypotheses[0].size(), size_t(10)); i++) {
        hypothesisStr += "[" + results[0].hypotheses[0][i] + "] ";
      }
      if (results[0].hypotheses[0].size() > 10) {
        hypothesisStr += "...";
      }
      AF_DEBUG("CTranslate2: First hypothesis tokens: {}", hypothesisStr);

      std::string translation = Detokenize(results[0].hypotheses[0]);

      AF_INFO("CTranslate2: Translation complete: '{}' -> '{}'", text, translation);

      return translation;

    } catch (const std::exception& e) {
      AF_ERROR("CTranslate2 translation failed: {}", e.what());
      return text;
    }
  }

  bool CTranslate2Translator::IsAvailable() const
  {
    return m_IsAvailable && m_Translator != nullptr;
  }

  void CTranslate2Translator::SetDevice(const std::string& device)
  {
    m_Device = device;
  }

  void CTranslate2Translator::SetBeamSize(size_t beamSize)
  {
    m_BeamSize = beamSize;
  }

  std::vector<std::string> CTranslate2Translator::Tokenize(const std::string& text) const
  {
    std::vector<std::string> tokens;

    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string utf32text;

    try {
      utf32text = converter.from_bytes(text);
    } catch (...) {
      AF_ERROR("CTranslate2: Failed to convert text to UTF-32");
      return tokens;
    }

    for (size_t i = 0; i < utf32text.size(); i++) {
      char32_t c = utf32text[i];
      std::string charStr = converter.to_bytes(c);

      if (i == 0) {
        tokens.push_back("▁" + charStr);
      } else if (c == U' ' || c == U'　') {
        tokens.push_back("▁");
      } else {
        tokens.push_back(charStr);
      }
    }

    return tokens;
  }

  std::string CTranslate2Translator::Detokenize(const std::vector<std::string>& tokens) const
  {
    std::string result;

    for (const auto& token : tokens) {
      if (token.empty() || token == "</s>") {
        continue;
      }

      if (token[0] == '\xe2' && token.size() >= 3 && token[1] == '\x96' && token[2] == '\x81') {
        result += " ";
        if (token.size() > 3) {
          result += token.substr(3);
        }
      } else {
        result += token;
      }
    }

    while (!result.empty() && result.back() == ' ') {
      result.pop_back();
    }

    while (!result.empty() && result.front() == ' ') {
      result.erase(0, 1);
    }

    return result;
  }

} // namespace Video2Card::Language::Translation
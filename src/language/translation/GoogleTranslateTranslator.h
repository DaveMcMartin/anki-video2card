#pragma once

#include <string>

#include "language/translation/ITranslator.h"

namespace Video2Card::Language::Translation
{

  class GoogleTranslateTranslator : public ITranslator
  {
public:

    explicit GoogleTranslateTranslator(std::string sourceLang = "ja",
                                       std::string targetLang = "en",
                                       int timeoutSeconds = 10);
    ~GoogleTranslateTranslator() override = default;

    [[nodiscard]] std::string Translate(const std::string& text) override;

    [[nodiscard]] bool IsAvailable() const override;

    void SetSourceLang(const std::string& lang) { m_SourceLang = lang; }
    void SetTargetLang(const std::string& lang) { m_TargetLang = lang; }

private:

    std::string m_SourceLang;
    std::string m_TargetLang;
    int m_TimeoutSeconds;
  };

} // namespace Video2Card::Language::Translation
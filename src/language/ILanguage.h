#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace Video2Card::Language
{

  class ILanguage
  {
public:

    virtual ~ILanguage() = default;

    virtual std::string GetName() const = 0;
    virtual std::string GetIdentifier() const = 0;
    virtual std::string GetLanguageCode() const = 0;
    virtual std::string GetFontPath() const = 0;

    virtual std::string GetOCRSystemPrompt() const = 0;
    virtual std::string GetOCRUserPrompt() const = 0;

    virtual std::string PostProcessOCR(const std::string& text) const { return text; }

    virtual std::string GetAnalysisSystemPrompt() const = 0;
    virtual std::string GetAnalysisUserPrompt(const std::string& sentence, const std::string& targetWord) const = 0;
    virtual std::string GetAnalysisOutputFormat() const = 0;

    virtual const void* GetImGuiFontGlyphRanges() const = 0;
  };

} // namespace Video2Card::Language

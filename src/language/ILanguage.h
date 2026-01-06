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

    virtual const void* GetImGuiFontGlyphRanges() const = 0;
  };

} // namespace Video2Card::Language

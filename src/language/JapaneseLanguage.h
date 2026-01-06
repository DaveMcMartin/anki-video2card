#pragma once

#include <imgui.h>

#include "language/ILanguage.h"

namespace Video2Card::Language
{

  class JapaneseLanguage : public ILanguage
  {
public:

    JapaneseLanguage() = default;
    ~JapaneseLanguage() override = default;

    std::string GetName() const override { return "Japanese"; }
    std::string GetIdentifier() const override { return "JP"; }
    std::string GetLanguageCode() const override { return "ja"; }
    std::string GetFontPath() const override { return "assets/NotoSansJP-Regular.otf"; }

    const void* GetImGuiFontGlyphRanges() const override { return ImGui::GetIO().Fonts->GetGlyphRangesJapanese(); }
  };

} // namespace Video2Card::Language

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

    std::string GetOCRSystemPrompt() const override
    {
      return "You are an expert OCR system specialized in Japanese text recognition. You extract text exactly as it "
             "appears, preserving accuracy.";
    }

    std::string GetOCRUserPrompt() const override
    {
      return R"(Extract ALL Japanese text from this image with high accuracy.

IMPORTANT INSTRUCTIONS:
- Japanese text can be written horizontally (left-to-right) OR vertically (top-to-bottom, right-to-left columns)
- Carefully identify the text direction before extracting
- For vertical text: read from top to bottom, then move to the next column on the left
- For horizontal text: read left to right, top to bottom
- Preserve the exact characters including kanji, hiragana, katakana
- Include all punctuation marks (。、！？「」etc.)
- Preserve line breaks and text order
- Do NOT translate, romanize, or add explanations
- Return ONLY the extracted Japanese text, nothing else

If multiple text blocks exist, extract them in reading order.)";
    }

    std::string PostProcessOCR(const std::string& text) const override
    {
      std::string result;
      result.reserve(text.size());
      for (char c : text) {
        // Remove spaces and newlines for Japanese text
        if (c != '\n' && c != '\r' && c != ' ') {
          result += c;
        }
      }
      return result;
    }

    std::string GetAnalysisSystemPrompt() const override
    {
      return "You are a helpful Japanese language learning assistant. You output valid JSON only.";
    }

    std::string GetAnalysisUserPrompt(const std::string& sentence, const std::string& targetWord) const override
    {
      std::string prompt = "Analyze the following Japanese sentence:\n\n" + sentence + "\n\n";

      if (!targetWord.empty()) {
        prompt += "Focus on the target word: " + targetWord + "\n\n";
      } else {
        prompt += "Identify the most important vocabulary word in this sentence as the target word.\n\n";
      }

      prompt += GetAnalysisOutputFormat();

      return prompt;
    }

    std::string GetAnalysisOutputFormat() const override
    {
      return R"(
Provide the output in strict JSON format with the following keys:
- "sentence": The original sentence with the target word highlighted in bold green HTML (e.g., "私 テーブル<b style="color: green;">拭く</b>ね").
- "translation": English translation of the sentence.
- "target_word": The dictionary form of the target word.
- "target_word_furigana": The target word with furigana in Anki format (e.g., 食[た]べる).
- "furigana": The sentence with furigana in Anki format. IMPORTANT: Add a space before and after each kanji in the sentence. Highlight the target word in bold green HTML (e.g., "私[わたし] テーブル<b style="color: green;"> 拭[ふ]く</b>ね。").
- "definition": The definition of the target word in English.
- "pitch_accent": The pitch accent pattern in HTML format. Use box-shadow styling with royalblue color for the accented mora and #FF6633 for the accent pattern. Include the pitch number at the end. Example: '<span style="box-shadow: inset -2px -2px 0 0 #FF6633;"><span style="color: royalblue;">フ</span></span><span style="box-shadow: inset 0px 2px 0 0px #FF6633;">ク</span> <span class="pitch_number">0</span>' for フク with 平板(0) pattern.

Do not include markdown formatting (like ```json). Just the raw JSON object.
)";
    }

    const void* GetImGuiFontGlyphRanges() const override { return ImGui::GetIO().Fonts->GetGlyphRangesJapanese(); }
  };

} // namespace Video2Card::Language

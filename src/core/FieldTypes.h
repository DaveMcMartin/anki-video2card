#pragma once

#include <array>
#include <string_view>

namespace Video2Card
{
  namespace Core
  {
    enum class FieldTool
    {
      SentenceText = 0,
      SentenceFurigana,
      SentenceTranslation,
      VocabWord,
      VocabFurigana,
      PitchAccent,
      VocabDefinition,
      Image,
      VocabAudio,
      SentenceAudio,
      Count
    };

    constexpr std::array<std::string_view, static_cast<size_t>(FieldTool::Count)> FieldToolNames = {
        "Sentence Text",
        "Sentence w/ Furigana",
        "Sentence Translation",
        "Vocab Word",
        "Vocab w/ Furigana",
        "Pitch Accent",
        "Vocab Definition",
        "Image",
        "Vocab Audio",
        "Sentence Audio"};
  } // namespace Core
} // namespace Video2Card

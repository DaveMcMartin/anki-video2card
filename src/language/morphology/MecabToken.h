#pragma once

#include <string>
#include <vector>

namespace Video2Card::Language::Morphology
{

  /**
 * Represents a single morphological token from Mecab analysis.
 * Contains all information about a word including its dictionary form,
 * reading, part of speech, and inflection information.
 */
  struct MecabToken
  {
    // The actual word/surface form as it appears in the text
    std::string surface;

    // The dictionary form (headword) of the word
    std::string headword;

    // The katakana reading of the word
    std::string katakanaReading;

    // Part of speech (e.g., "noun", "verb", "adjective")
    std::string partOfSpeech;

    // Subclass of POS (e.g., "general", "proper noun")
    std::string posSubclass1;
    std::string posSubclass2;
    std::string posSubclass3;

    // Inflection type (e.g., "i-adjective", "consonant stem")
    std::string inflectionType;

    // Inflection form (e.g., "base form", "te form")
    std::string inflectionForm;

    MecabToken() = default;

    MecabToken(std::string surface_, std::string headword_, std::string katakanaReading_, std::string partOfSpeech_)
        : surface(std::move(surface_))
        , headword(std::move(headword_))
        , katakanaReading(std::move(katakanaReading_))
        , partOfSpeech(std::move(partOfSpeech_))
    {}
  };

  using MecabTokenList = std::vector<MecabToken>;

} // namespace Video2Card::Language::Morphology
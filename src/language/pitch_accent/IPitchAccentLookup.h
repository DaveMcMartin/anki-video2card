#pragma once

#include <string>
#include <vector>

namespace Video2Card::Language::PitchAccent
{

  struct PitchAccentEntry
  {
    std::string headword;
    std::string katakanaReading;
    std::string htmlNotation;
    std::string pitchNumber;
  };

  class IPitchAccentLookup
  {
public:

    virtual ~IPitchAccentLookup() = default;

    [[nodiscard]] virtual std::vector<PitchAccentEntry> LookupWord(const std::string& word,
                                                                   const std::string& reading = "") = 0;

    [[nodiscard]] virtual std::string FormatAsHtml(const std::vector<PitchAccentEntry>& entries) = 0;

    [[nodiscard]] virtual bool IsAvailable() const = 0;
  };

} // namespace Video2Card::Language::PitchAccent
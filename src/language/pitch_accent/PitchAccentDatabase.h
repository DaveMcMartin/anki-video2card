#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IPitchAccentLookup.h"

struct sqlite3;

namespace Video2Card::Language::PitchAccent
{

  class PitchAccentDatabase : public IPitchAccentLookup
  {
public:

    explicit PitchAccentDatabase(const std::string& dbPath);
    ~PitchAccentDatabase() override;

    PitchAccentDatabase(const PitchAccentDatabase&) = delete;
    PitchAccentDatabase& operator=(const PitchAccentDatabase&) = delete;
    PitchAccentDatabase(PitchAccentDatabase&&) = delete;
    PitchAccentDatabase& operator=(PitchAccentDatabase&&) = delete;

    [[nodiscard]] std::vector<PitchAccentEntry> LookupWord(const std::string& word,
                                                           const std::string& reading = "") override;

    [[nodiscard]] std::string FormatAsHtml(const std::vector<PitchAccentEntry>& entries) override;

    [[nodiscard]] bool IsAvailable() const override;

private:

    [[nodiscard]] std::string ConvertXmlTagsToHtml(const std::string& xmlNotation) const;

    sqlite3* m_Database;
    std::string m_DatabasePath;
  };

} // namespace Video2Card::Language::PitchAccent
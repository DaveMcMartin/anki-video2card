#include "PitchAccentDatabase.h"

#include <regex>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

#include "core/Logger.h"

namespace Video2Card::Language::PitchAccent
{

  PitchAccentDatabase::PitchAccentDatabase(const std::string& dbPath)
      : m_Database(nullptr)
      , m_DatabasePath(dbPath)
  {
    int result = sqlite3_open(dbPath.c_str(), &m_Database);

    if (result != SQLITE_OK) {
      std::string error = sqlite3_errmsg(m_Database);
      sqlite3_close(m_Database);
      m_Database = nullptr;
      throw std::runtime_error("Failed to open pitch accent database: " + error);
    }

    AF_INFO("PitchAccentDatabase initialized with database: {}", dbPath);
  }

  PitchAccentDatabase::~PitchAccentDatabase()
  {
    if (m_Database) {
      sqlite3_close(m_Database);
      m_Database = nullptr;
    }
  }

  std::vector<PitchAccentEntry> PitchAccentDatabase::LookupWord(const std::string& word, const std::string& reading)
  {
    std::vector<PitchAccentEntry> results;

    if (!IsAvailable()) {
      AF_WARN("Pitch accent database not available");
      return results;
    }

    if (word.empty()) {
      return results;
    }

    const char* sql = R"(
      SELECT DISTINCT raw_headword, katakana_reading, html_notation, pitch_number
      FROM pitch_accents_formatted
      WHERE (headword = ? OR katakana_reading = ?)
      ORDER BY frequency DESC, pitch_number ASC, katakana_reading ASC
      LIMIT 10
    )";

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(m_Database, sql, -1, &stmt, nullptr);

    if (result != SQLITE_OK) {
      AF_ERROR("Failed to prepare pitch accent lookup query: {}", sqlite3_errmsg(m_Database));
      return results;
    }

    std::string searchWord = reading.empty() ? word : reading;
    sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, searchWord.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      PitchAccentEntry entry;

      const char* raw_headword = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      const char* katakana = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      const char* html = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
      const char* pitch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

      if (raw_headword) {
        entry.headword = raw_headword;
      }
      if (katakana) {
        entry.katakanaReading = katakana;
      }
      if (html) {
        entry.htmlNotation = html;
      }
      if (pitch) {
        entry.pitchNumber = pitch;
      }

      results.push_back(entry);
    }

    sqlite3_finalize(stmt);

    AF_DEBUG("Found {} pitch accent entries for word: {}", results.size(), word);

    return results;
  }

  std::string PitchAccentDatabase::FormatAsHtml(const std::vector<PitchAccentEntry>& entries)
  {
    if (entries.empty()) {
      return "";
    }

    std::ostringstream result;

    for (size_t i = 0; i < entries.size(); ++i) {
      const auto& entry = entries[i];

      std::string htmlNotation = ConvertXmlTagsToHtml(entry.htmlNotation);

      result << htmlNotation;

      if (!entry.pitchNumber.empty()) {
        result << " <span class=\"pitch_number\">" << entry.pitchNumber << "</span>";
      }

      if (i < entries.size() - 1) {
        result << "・";
      }
    }

    return result.str();
  }

  bool PitchAccentDatabase::IsAvailable() const
  {
    return m_Database != nullptr;
  }

  std::string PitchAccentDatabase::ConvertXmlTagsToHtml(const std::string& xmlNotation) const
  {
    std::string result = xmlNotation;

    static const std::vector<std::pair<std::string, std::string>> replacements = {
        {"<low_rise>", "<span style=\"box-shadow: inset -2px -2px 0 0 #FF6633;\">"},
        {"</low_rise>", "</span>"},
        {"<low>", "<span style=\"box-shadow: inset 0px -2px 0 0px #FF6633;\">"},
        {"</low>", "</span>"},
        {"<high>", "<span style=\"box-shadow: inset 0px 2px 0 0px #FF6633;\">"},
        {"</high>", "</span>"},
        {"<high_drop>", "<span style=\"box-shadow: inset -2px 2px 0 0px #FF6633;\">"},
        {"</high_drop>", "</span>"},
        {"<devoiced>", "<span style=\"color: royalblue;\">"},
        {"</devoiced>", "</span>"},
        {"<nasal>", ""},
        {"</nasal>", ""},
        {"<handakuten>", "<span style=\"color: red;\">"},
        {"</handakuten>", "</span>"}};

    for (const auto& [from, to] : replacements) {
      size_t pos = 0;
      while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
      }
    }

    return result;
  }

} // namespace Video2Card::Language::PitchAccent
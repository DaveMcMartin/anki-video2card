#include "JMDictionary.h"

#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

#include "core/Logger.h"

namespace Video2Card::Language::Dictionary
{

  JMDictionary::JMDictionary(const std::string& dbPath)
      : m_Database(nullptr)
      , m_DatabasePath(dbPath)
  {
    int result = sqlite3_open(dbPath.c_str(), &m_Database);

    if (result != SQLITE_OK) {
      std::string error = sqlite3_errmsg(m_Database);
      sqlite3_close(m_Database);
      m_Database = nullptr;
      throw std::runtime_error("Failed to open JMDict database: " + error);
    }

    AF_INFO("Initialized JMDict local dictionary from: {}", dbPath);
  }

  JMDictionary::~JMDictionary()
  {
    if (m_Database) {
      sqlite3_close(m_Database);
      m_Database = nullptr;
    }
  }

  DictionaryEntry JMDictionary::LookupWord(const std::string& word, const std::string& headword)
  {
    if (word.empty()) {
      return DictionaryEntry();
    }

    if (!IsAvailable()) {
      AF_WARN("JMDict database not available");
      return DictionaryEntry();
    }

    std::string lookupWord = !headword.empty() ? headword : word;

    std::vector<LookupResult> results = LookupByKanji(lookupWord);

    if (results.empty()) {
      results = LookupByReading(lookupWord);
    }

    if (results.empty()) {
      AF_DEBUG("No definition found for word: {}", lookupWord);
      return DictionaryEntry();
    }

    std::string definition = FormatDefinition(results);
    return DictionaryEntry(lookupWord, definition);
  }

  bool JMDictionary::IsAvailable() const
  {
    return m_Database != nullptr;
  }

  std::vector<JMDictionary::LookupResult> JMDictionary::LookupByKanji(const std::string& word)
  {
    std::vector<LookupResult> results;

    const char* sql = R"(
      SELECT DISTINCT r.reb, s.pos, s.gloss
      FROM kanji_elements k
      JOIN entries e ON k.entry_id = e.id
      JOIN reading_elements r ON r.entry_id = e.id
      JOIN senses s ON s.entry_id = e.id
      WHERE k.keb = ?
      LIMIT 10
    )";

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(m_Database, sql, -1, &stmt, nullptr);

    if (result != SQLITE_OK) {
      AF_ERROR("Failed to prepare kanji lookup query: {}", sqlite3_errmsg(m_Database));
      return results;
    }

    sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      LookupResult lookupResult;

      const char* reading = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      const char* pos = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      const char* gloss = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

      if (reading) {
        lookupResult.reading = reading;
      }
      if (pos) {
        lookupResult.pos = pos;
      }
      if (gloss) {
        lookupResult.gloss = gloss;
      }

      results.push_back(lookupResult);
    }

    sqlite3_finalize(stmt);
    return results;
  }

  std::vector<JMDictionary::LookupResult> JMDictionary::LookupByReading(const std::string& word)
  {
    std::vector<LookupResult> results;

    const char* sql = R"(
      SELECT DISTINCT r.reb, s.pos, s.gloss
      FROM reading_elements r
      JOIN entries e ON r.entry_id = e.id
      JOIN senses s ON s.entry_id = e.id
      WHERE r.reb = ?
      LIMIT 10
    )";

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(m_Database, sql, -1, &stmt, nullptr);

    if (result != SQLITE_OK) {
      AF_ERROR("Failed to prepare reading lookup query: {}", sqlite3_errmsg(m_Database));
      return results;
    }

    sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      LookupResult lookupResult;

      const char* reading = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      const char* pos = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      const char* gloss = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

      if (reading) {
        lookupResult.reading = reading;
      }
      if (pos) {
        lookupResult.pos = pos;
      }
      if (gloss) {
        lookupResult.gloss = gloss;
      }

      results.push_back(lookupResult);
    }

    sqlite3_finalize(stmt);
    return results;
  }

  std::string JMDictionary::FormatDefinition(const std::vector<LookupResult>& results)
  {
    if (results.empty()) {
      return "";
    }

    std::ostringstream oss;

    for (size_t i = 0; i < results.size() && i < 5; ++i) {
      const auto& result = results[i];

      if (i > 0) {
        oss << " | ";
      }

      if (!result.gloss.empty()) {
        oss << result.gloss;
      }
    }

    return oss.str();
  }

} // namespace Video2Card::Language::Dictionary
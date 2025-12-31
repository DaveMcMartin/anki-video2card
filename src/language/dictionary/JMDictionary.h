#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IDictionaryClient.h"

struct sqlite3;

namespace Video2Card::Language::Dictionary
{

  class JMDictionary : public IDictionaryClient
  {
public:

    explicit JMDictionary(const std::string& dbPath);
    ~JMDictionary() override;

    JMDictionary(const JMDictionary&) = delete;
    JMDictionary& operator=(const JMDictionary&) = delete;
    JMDictionary(JMDictionary&&) = delete;
    JMDictionary& operator=(JMDictionary&&) = delete;

    [[nodiscard]] DictionaryEntry LookupWord(const std::string& word, const std::string& headword = "") override;

    [[nodiscard]] bool IsAvailable() const override;

private:

    struct LookupResult
    {
      std::string reading;
      std::string pos;
      std::string gloss;
    };

    [[nodiscard]] std::vector<LookupResult> LookupByKanji(const std::string& word);
    [[nodiscard]] std::vector<LookupResult> LookupByReading(const std::string& word);
    [[nodiscard]] std::string FormatDefinition(const std::vector<LookupResult>& results);

    sqlite3* m_Database;
    std::string m_DatabasePath;
  };

} // namespace Video2Card::Language::Dictionary
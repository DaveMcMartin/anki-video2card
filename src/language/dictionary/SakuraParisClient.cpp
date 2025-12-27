#include "SakuraParisClient.h"

#include <exception>
#include <httplib.h>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "core/Logger.h"

namespace Video2Card::Language::Dictionary
{

  SakuraParisClient::SakuraParisClient(Dictionary dictionary, SearchType searchType, int timeoutSeconds)
      : m_Dictionary(dictionary)
      , m_SearchType(searchType)
      , m_TimeoutSeconds(timeoutSeconds)
  {
    AF_INFO("Initialized Sakura-Paris dictionary client");
  }

  DictionaryEntry SakuraParisClient::LookupWord(const std::string& word, const std::string& headword)
  {
    if (word.empty()) {
      return DictionaryEntry();
    }

    // Use headword if provided, otherwise use word
    std::string lookupWord = !headword.empty() ? headword : word;

    try {
      std::string definition = FetchDefinition(lookupWord);

      if (definition.empty()) {
        AF_WARN("No definition found for word: {}", lookupWord);
        return DictionaryEntry();
      }

      return DictionaryEntry(lookupWord, definition);
    } catch (const std::exception& e) {
      AF_ERROR("Dictionary lookup failed for '{}': {}", lookupWord, e.what());
      return DictionaryEntry();
    }
  }

  bool SakuraParisClient::IsAvailable() const
  {
    try {
      httplib::Client cli("sakura-paris.org");
      cli.set_connection_timeout(2, 0);
      auto res = cli.Head("/");
      return res && res->status == 200;
    } catch (...) {
      return false;
    }
  }

  void SakuraParisClient::SetDictionary(Dictionary dictionary)
  {
    m_Dictionary = dictionary;
  }

  void SakuraParisClient::SetSearchType(SearchType searchType)
  {
    m_SearchType = searchType;
  }

  std::string SakuraParisClient::DictionaryToString(Dictionary dict)
  {
    switch (dict) {
      case Dictionary::Daijirin:
        return "大辞林";
      case Dictionary::Daijisen:
        return "大辞泉";
      case Dictionary::Meikyou:
        return "明鏡国語辞典";
      case Dictionary::Shinmeikai:
        return "新明解国語辞典";
      case Dictionary::Koujien:
        return "広辞苑";
      default:
        return "大辞林";
    }
  }

  std::string SakuraParisClient::SearchTypeToString(SearchType type)
  {
    switch (type) {
      case SearchType::Exact:
        return "exact";
      case SearchType::Prefix:
        return "prefix";
      case SearchType::Suffix:
        return "suffix";
      default:
        return "exact";
    }
  }

  std::string SakuraParisClient::FetchDefinition(const std::string& word)
  {
    try {
      // Build the URL
      std::string dictStr = DictionaryToString(m_Dictionary);
      std::string searchStr = SearchTypeToString(m_SearchType);

      std::string path = "/dict/" + dictStr + "/" + searchStr + "/" + word;

      httplib::Client cli("sakura-paris.org");
      cli.set_connection_timeout(m_TimeoutSeconds, 0);
      cli.set_follow_location(true);

      auto res = cli.Get(path.c_str());

      if (!res) {
        AF_WARN("HTTP request failed for word '{}': no response", word);
        return "";
      }

      if (res->status != 200) {
        AF_WARN("Dictionary lookup returned status {} for word '{}'", res->status, word);
        return "";
      }

      return ParseDefinitionFromHtml(res->body);
    } catch (const std::exception& e) {
      AF_ERROR("Exception during dictionary fetch: {}", e.what());
      return "";
    }
  }

  std::string SakuraParisClient::ParseDefinitionFromHtml(const std::string& html)
  {
    if (html.empty()) {
      return "";
    }

    // Simple HTML parsing: look for content between <div class="content"> tags
    std::regex contentRegex(R"(<div class="content">(.+?)</div>)", std::regex::icase);
    std::smatch match;

    if (std::regex_search(html, match, contentRegex)) {
      std::string content = match[1].str();

      // Remove HTML tags
      std::regex tagRegex(R"(<[^>]+>)");
      content = std::regex_replace(content, tagRegex, "");

      // Replace HTML entities
      content = std::regex_replace(content, std::regex("&nbsp;"), " ");
      content = std::regex_replace(content, std::regex("&lt;"), "<");
      content = std::regex_replace(content, std::regex("&gt;"), ">");
      content = std::regex_replace(content, std::regex("&amp;"), "&");
      content = std::regex_replace(content, std::regex("&quot;"), "\"");

      // Trim whitespace
      auto start = content.find_first_not_of(" \t\n\r");
      auto end = content.find_last_not_of(" \t\n\r");
      if (start != std::string::npos && end != std::string::npos) {
        return content.substr(start, end - start + 1);
      }
    }

    return "";
  }

} // namespace Video2Card::Language::Dictionary
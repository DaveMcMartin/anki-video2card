#pragma once

#include <memory>
#include <string>

#include "IDictionaryClient.h"

namespace Video2Card::Language::Dictionary
{

  /**
 * Dictionary client using Sakura-Paris online dictionary.
 * Provides free access to multiple Japanese dictionaries via web scraping.
 * Supports: 大辞林, 大辞泉, 明鏡国語辞典, 新明解国語辞典, 広辞苑
 */
  class SakuraParisClient : public IDictionaryClient
  {
public:

    enum class Dictionary
    {
      Daijirin,   // 大辞林
      Daijisen,   // 大辞泉
      Meikyou,    // 明鏡国語辞典
      Shinmeikai, // 新明解国語辞典
      Koujien     // 広辞苑
    };

    enum class SearchType
    {
      Exact,  // 完全一致
      Prefix, // 前方一致
      Suffix  // 後方一致
    };

    /**
   * Create a Sakura-Paris dictionary client.
   * @param dictionary The dictionary to use for lookups
   * @param searchType The search type to use
   * @param timeoutSeconds Timeout for HTTP requests
   */
    SakuraParisClient(Dictionary dictionary = Dictionary::Daijirin,
                      SearchType searchType = SearchType::Exact,
                      int timeoutSeconds = 10);

    ~SakuraParisClient() override = default;

    DictionaryEntry LookupWord(const std::string& word, const std::string& headword = "") override;

    [[nodiscard]] bool IsAvailable() const override;

    /**
   * Set the dictionary to use for lookups.
   * @param dictionary The dictionary enum value
   */
    void SetDictionary(Dictionary dictionary);

    /**
   * Set the search type for lookups.
   * @param searchType The search type enum value
   */
    void SetSearchType(SearchType searchType);

private:

    /**
   * Convert Dictionary enum to string representation.
   */
    [[nodiscard]] static std::string DictionaryToString(Dictionary dict);

    /**
   * Convert SearchType enum to string representation.
   */
    [[nodiscard]] static std::string SearchTypeToString(SearchType type);

    /**
   * Fetch and parse definition from Sakura-Paris.
   * @param word The word to look up
   * @return The definition HTML, or empty if not found
   */
    [[nodiscard]] std::string FetchDefinition(const std::string& word);

    /**
   * Parse HTML definition and extract text content.
   * @param html The HTML content from Sakura-Paris
   * @return Clean text definition
   */
    [[nodiscard]] static std::string ParseDefinitionFromHtml(const std::string& html);

    Dictionary m_Dictionary;
    SearchType m_SearchType;
    int m_TimeoutSeconds;
    std::string m_BaseUrl = "https://sakura-paris.org";
  };

} // namespace Video2Card::Language::Dictionary
#pragma once

#include <string>
#include <vector>

namespace Video2Card::Language::Dictionary
{

  /**
 * Dictionary lookup result containing definition and metadata.
 */
  struct DictionaryEntry
  {
    std::string headword;     // The dictionary form of the word
    std::string definition;   // The definition text
    std::string partOfSpeech; // Part of speech (noun, verb, etc.)
    std::string example;      // Example usage (if available)

    DictionaryEntry() = default;

    DictionaryEntry(std::string hw, std::string def)
        : headword(std::move(hw))
        , definition(std::move(def))
    {}
  };

  /**
 * Interface for Japanese dictionary lookups.
 * Provides definitions and other dictionary information for Japanese words.
 */
  class IDictionaryClient
  {
public:

    virtual ~IDictionaryClient() = default;

    /**
   * Look up a word in the dictionary.
   * @param word The word to look up (can be in any form)
   * @param headword The dictionary form of the word (for more accurate lookup)
   * @return Dictionary entry with definition, or empty if not found
   * @throws std::runtime_error if lookup fails
   */
    [[nodiscard]] virtual DictionaryEntry LookupWord(const std::string& word, const std::string& headword = "") = 0;

    /**
   * Check if the dictionary is available and ready to use.
   * @return true if the dictionary is accessible
   */
    [[nodiscard]] virtual bool IsAvailable() const = 0;
  };

} // namespace Video2Card::Language::Dictionary
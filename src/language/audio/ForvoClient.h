#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IAudioSource.h"

namespace Video2Card::Language::Audio
{

  /**
 * Forvo audio source client.
 * Forvo (https://forvo.com) is a pronunciation dictionary with user-submitted
 * audio recordings in multiple languages including Japanese.
 * 
 * This implementation uses web scraping to find audio files since Forvo
 * requires API key for their official API.
 */
  class ForvoClient : public IAudioSource
  {
public:

    /**
   * Create a Forvo client for Japanese audio.
   * @param language Language code (default: "ja" for Japanese)
   * @param timeoutSeconds HTTP request timeout
   * @param maxResults Maximum number of results to return per search
   */
    explicit ForvoClient(std::string language = "ja", int timeoutSeconds = 10, int maxResults = 5);

    ~ForvoClient() override = default;

    /**
   * Search Forvo for audio pronunciations of a word.
   * @param word The word to search for
   * @param headword The dictionary form (used if word is empty)
   * @param reading Optional reading (not used by Forvo)
   * @return List of audio files found
   */
    [[nodiscard]] std::vector<AudioFileInfo>
    SearchAudio(const std::string& word, const std::string& headword = "", const std::string& reading = "") override;

    /**
   * Get the name of this audio source.
   * @return "Forvo"
   */
    [[nodiscard]] std::string GetName() const override;

    /**
   * Check if Forvo is available.
   * @return true if forvo.com is accessible
   */
    [[nodiscard]] bool IsAvailable() const override;

    /**
   * Set preferred usernames for filtering results.
   * Results from these users will be prioritized.
   * @param usernames Comma-separated list of usernames
   */
    void SetPreferredUsernames(const std::string& usernames);

    /**
   * Set preferred countries for filtering results.
   * @param countries Comma-separated list of countries (e.g., "Japan")
   */
    void SetPreferredCountries(const std::string& countries);

    /**
   * Set preferred audio format.
   * @param format "mp3" or "ogg" (default: "mp3")
   */
    void SetAudioFormat(const std::string& format);

private:

    /**
   * Fetch the word page from Forvo.
   * @param word The word to look up
   * @return HTML content of the page
   */
    [[nodiscard]] std::string FetchWordPage(const std::string& word) const;

    /**
   * Fetch the search page from Forvo (fallback when word page fails).
   * @param word The word to search for
   * @return HTML content of the page
   */
    [[nodiscard]] std::string FetchSearchPage(const std::string& word) const;

    /**
   * Parse Forvo HTML page and extract audio URLs.
   * @param html The HTML content
   * @param word The original word being searched
   * @return List of audio file information
   */
    [[nodiscard]] std::vector<AudioFileInfo> ParseAudioLinks(const std::string& html, const std::string& word) const;

    /**
   * Extract audio URL from Forvo's JavaScript-encoded format.
   * Forvo often uses base64 or other encoding for audio URLs.
   * @param encodedData The encoded audio data
   * @return Decoded audio URL
   */
    [[nodiscard]] std::string DecodeAudioUrl(const std::string& encodedData) const;

    /**
   * Filter and sort results based on preferences.
   * @param results The raw search results
   * @return Filtered and sorted results
   */
    [[nodiscard]] std::vector<AudioFileInfo> FilterResults(std::vector<AudioFileInfo> results) const;

    /**
   * Generate a filename for the audio file.
   * @param word The word
   * @param username The Forvo username
   * @param index Sequential index
   * @return Filename like "word_forvo_user_1.mp3"
   */
    [[nodiscard]] std::string GenerateFilename(const std::string& word,
                                               const std::string& username,
                                               int index,
                                               const std::string& extension) const;

    std::string m_Language;
    int m_TimeoutSeconds;
    int m_MaxResults;
    std::string m_PreferredUsernames;
    std::string m_PreferredCountries;
    std::string m_AudioFormat;
    std::string m_BaseUrl;
  };

} // namespace Video2Card::Language::Audio
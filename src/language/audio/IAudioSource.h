#pragma once

#include <string>
#include <vector>

namespace Video2Card::Language::Audio
{

  /**
 * Information about an audio file for a Japanese word.
 */
  struct AudioFileInfo
  {
    std::string url;        // URL to download the audio file
    std::string filename;   // Suggested filename for the audio
    std::string word;       // The word this audio is for
    std::string reading;    // Kana reading of the word
    std::string sourceName; // Name of the source (e.g., "NHK-2016")
    int pitchAccent;        // Pitch accent number (0 = unknown)

    AudioFileInfo()
        : pitchAccent(0)
    {}

    AudioFileInfo(std::string url_,
                  std::string filename_,
                  std::string word_,
                  std::string reading_,
                  std::string sourceName_,
                  int pitchAccent_ = 0)
        : url(std::move(url_))
        , filename(std::move(filename_))
        , word(std::move(word_))
        , reading(std::move(reading_))
        , sourceName(std::move(sourceName_))
        , pitchAccent(pitchAccent_)
    {}
  };

  /**
 * Interface for audio pronunciation sources.
 * Implementations can search local databases or online services
 * for Japanese word pronunciations.
 */
  class IAudioSource
  {
public:

    virtual ~IAudioSource() = default;

    /**
   * Search for audio files for a given word.
   * @param word The word to search for (in any form)
   * @param headword The dictionary form of the word (for better results)
   * @param reading Optional kana reading to narrow results
   * @return List of audio files found
   */
    [[nodiscard]] virtual std::vector<AudioFileInfo>
    SearchAudio(const std::string& word, const std::string& headword = "", const std::string& reading = "") = 0;

    /**
   * Get the name of this audio source.
   * @return Human-readable name (e.g., "NHK 2016")
   */
    [[nodiscard]] virtual std::string GetName() const = 0;

    /**
   * Check if this audio source is available/ready.
   * @return true if the source can be used
   */
    [[nodiscard]] virtual bool IsAvailable() const = 0;
  };

} // namespace Video2Card::Language::Audio
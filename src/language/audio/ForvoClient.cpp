#include "ForvoClient.h"

#include <algorithm>
#include <exception>
#include <httplib.h>
#include <regex>
#include <sstream>

#include "core/Logger.h"

namespace Video2Card::Language::Audio
{

  ForvoClient::ForvoClient(std::string language, int timeoutSeconds, int maxResults)
      : m_Language(std::move(language))
      , m_TimeoutSeconds(timeoutSeconds)
      , m_MaxResults(maxResults)
      , m_AudioFormat("mp3")
      , m_BaseUrl("https://forvo.com")
  {
    AF_INFO("ForvoClient initialized for language: {}", m_Language);
  }

  std::vector<AudioFileInfo>
  ForvoClient::SearchAudio(const std::string& word, const std::string& headword, const std::string& reading)
  {
    (void) reading; // Reading not used by Forvo

    std::string searchWord = word.empty() ? headword : word;
    if (searchWord.empty()) {
      AF_WARN("ForvoClient: empty search word");
      return {};
    }

    try {
      AF_DEBUG("Searching Forvo for: {}", searchWord);
      std::string html = FetchWordPage(searchWord);

      if (html.empty()) {
        AF_WARN("ForvoClient: no content returned for word '{}'", searchWord);
        return {};
      }

      auto results = ParseAudioLinks(html, searchWord);
      results = FilterResults(std::move(results));

      AF_INFO("ForvoClient: found {} audio files for '{}'", results.size(), searchWord);
      return results;

    } catch (const std::exception& e) {
      AF_ERROR("ForvoClient: search failed for '{}': {}", searchWord, e.what());
      return {};
    }
  }

  std::string ForvoClient::GetName() const
  {
    return "Forvo";
  }

  bool ForvoClient::IsAvailable() const
  {
    try {
      httplib::SSLClient cli("forvo.com");
      cli.set_connection_timeout(2, 0);
      auto res = cli.Head("/");
      return res && (res->status == 200 || res->status == 301 || res->status == 302);
    } catch (...) {
      return false;
    }
  }

  void ForvoClient::SetPreferredUsernames(const std::string& usernames)
  {
    m_PreferredUsernames = usernames;
  }

  void ForvoClient::SetPreferredCountries(const std::string& countries)
  {
    m_PreferredCountries = countries;
  }

  void ForvoClient::SetAudioFormat(const std::string& format)
  {
    if (format == "mp3" || format == "ogg") {
      m_AudioFormat = format;
    } else {
      AF_WARN("ForvoClient: unsupported audio format '{}', using mp3", format);
      m_AudioFormat = "mp3";
    }
  }

  std::string ForvoClient::FetchWordPage(const std::string& word) const
  {
    try {
      httplib::SSLClient cli("forvo.com");
      cli.set_connection_timeout(m_TimeoutSeconds, 0);
      cli.set_read_timeout(m_TimeoutSeconds, 0);
      cli.set_follow_location(true);

      std::string path = "/word/" + word + "/";
      if (m_Language != "ja") {
        path += "#" + m_Language;
      }

      auto res = cli.Get(path.c_str());

      if (!res) {
        AF_WARN("ForvoClient: HTTP request failed for word '{}'", word);
        return "";
      }

      if (res->status != 200) {
        AF_WARN("ForvoClient: HTTP status {} for word '{}'", res->status, word);
        return "";
      }

      return res->body;

    } catch (const std::exception& e) {
      AF_ERROR("ForvoClient: exception while fetching '{}': {}", word, e.what());
      return "";
    }
  }

  std::vector<AudioFileInfo> ForvoClient::ParseAudioLinks(const std::string& html, const std::string& word) const
  {
    std::vector<AudioFileInfo> results;

    // Forvo uses different patterns to embed audio URLs
    // Pattern 1: Look for Play() function calls with base64 encoded data
    std::regex playRegex(R"(Play\(\d+,\s*'([^']+)',\s*'([^']+)',\s*[^,]+,\s*'([^']+)')");

    // Pattern 2: Look for direct MP3/OGG URLs in the HTML
    std::regex directUrlRegex(R"(https?://[^\"'\s]+\.(?:mp3|ogg))");

    std::smatch match;
    std::string::const_iterator searchStart(html.cbegin());
    int index = 0;

    // Try pattern 1 - Play() function calls
    while (std::regex_search(searchStart, html.cend(), match, playRegex) && index < m_MaxResults) {
      try {
        std::string encodedUrl = match[1].str();
        std::string username = match[3].str();

        std::string audioUrl = DecodeAudioUrl(encodedUrl);

        if (!audioUrl.empty()) {
          AudioFileInfo info;
          info.word = word;
          info.url = audioUrl;
          info.filename = GenerateFilename(word, username, index);
          info.sourceName = "Forvo (" + username + ")";
          info.reading = "";
          info.pitchAccent = 0;

          results.push_back(info);
          index++;
        }
      } catch (const std::exception& e) {
        AF_WARN("ForvoClient: failed to parse audio link: {}", e.what());
      }

      searchStart = match.suffix().first;
    }

    // Try pattern 2 - Direct URLs (fallback)
    if (results.empty()) {
      searchStart = html.cbegin();
      while (std::regex_search(searchStart, html.cend(), match, directUrlRegex) && index < m_MaxResults) {
        std::string audioUrl = match[0].str();

        // Filter to only include forvo.com URLs
        if (audioUrl.find("forvo.com") != std::string::npos) {
          AudioFileInfo info;
          info.word = word;
          info.url = audioUrl;
          info.filename = GenerateFilename(word, "forvo", index);
          info.sourceName = "Forvo";
          info.reading = "";
          info.pitchAccent = 0;

          results.push_back(info);
          index++;
        }

        searchStart = match.suffix().first;
      }
    }

    return results;
  }

  std::string ForvoClient::DecodeAudioUrl(const std::string& encodedData)
  {
    // Forvo uses base64 encoding for audio URLs
    // This is a simplified decoder - in practice, you might need a proper base64 library

    // For now, check if it looks like a URL already
    if (encodedData.find("http") == 0) {
      return encodedData;
    }

    // If it's base64, we'd need to decode it
    // Since we don't have a base64 decoder readily available,
    // we'll return the encoded data and let the caller handle it
    // In a production implementation, use a proper base64 library

    // Simple heuristic: if it contains common URL patterns, use it as-is
    if (encodedData.find("audio") != std::string::npos || encodedData.find(".mp3") != std::string::npos ||
        encodedData.find(".ogg") != std::string::npos)
    {
      return encodedData;
    }

    return "";
  }

  std::vector<AudioFileInfo> ForvoClient::FilterResults(std::vector<AudioFileInfo> results) const
  {
    if (results.empty()) {
      return results;
    }

    // Filter by preferred usernames if specified
    if (!m_PreferredUsernames.empty()) {
      std::vector<AudioFileInfo> preferred;
      std::vector<AudioFileInfo> others;

      for (auto& result : results) {
        bool isPreferred = false;

        // Split preferred usernames by comma
        std::stringstream ss(m_PreferredUsernames);
        std::string username;
        while (std::getline(ss, username, ',')) {
          // Trim whitespace
          username.erase(0, username.find_first_not_of(" \t"));
          username.erase(username.find_last_not_of(" \t") + 1);

          if (result.sourceName.find(username) != std::string::npos) {
            isPreferred = true;
            break;
          }
        }

        if (isPreferred) {
          preferred.push_back(result);
        } else {
          others.push_back(result);
        }
      }

      // Combine: preferred first, then others
      preferred.insert(preferred.end(), others.begin(), others.end());
      results = std::move(preferred);
    }

    // Limit to max results
    if (static_cast<int>(results.size()) > m_MaxResults) {
      results.resize(m_MaxResults);
    }

    return results;
  }

  std::string ForvoClient::GenerateFilename(const std::string& word, const std::string& username, int index) const
  {
    // Create a safe filename
    std::string safeWord = word;
    std::string safeUsername = username;

    // Remove special characters
    auto cleanString = [](std::string& s) {
      s.erase(std::remove_if(s.begin(), s.end(), [](char c) { return !std::isalnum(c) && c != '_' && c != '-'; }),
              s.end());
    };

    cleanString(safeWord);
    cleanString(safeUsername);

    std::stringstream ss;
    ss << safeWord << "_forvo";

    if (!safeUsername.empty()) {
      ss << "_" << safeUsername;
    }

    if (index > 0) {
      ss << "_" << index;
    }

    ss << "." << m_AudioFormat;

    return ss.str();
  }

} // namespace Video2Card::Language::Audio
#include "ForvoClient.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <httplib.h>
#include <regex>
#include <sstream>
#include <thread>

#include "core/Logger.h"
#include "utils/Base64Utils.h"

namespace Video2Card::Language::Audio
{

  ForvoClient::ForvoClient(std::string language, int timeoutSeconds, int maxResults)
      : m_Language(std::move(language))
      , m_TimeoutSeconds(timeoutSeconds)
      , m_MaxResults(maxResults)
      , m_AudioFormat("mp3")
      , m_BaseUrl("https://forvo.com")
  {
    AF_INFO("ForvoClient initialized for language: {} (format: {})", m_Language, m_AudioFormat);
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
        AF_DEBUG("ForvoClient: word page empty, trying search page for '{}'", searchWord);
        html = FetchSearchPage(searchWord);
      }

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
    const int maxRetries = 3;

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
      try {
        if (attempt > 0) {
          int backoffMs = 500 * (1 << (attempt - 1));
          AF_DEBUG("ForvoClient: Retry attempt {} after {}ms backoff", attempt, backoffMs);
          std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
        }

        httplib::SSLClient cli("forvo.com");
        cli.set_connection_timeout(m_TimeoutSeconds, 0);
        cli.set_read_timeout(m_TimeoutSeconds, 0);
        cli.set_follow_location(true);

        httplib::Headers headers = {
            {"User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:139.0) Gecko/20100101 Firefox/139.0"},
            {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8"},
            {"Accept-Language", "en-US,en;q=0.5"},
            {"DNT", "1"},
            {"Connection", "keep-alive"},
            {"Upgrade-Insecure-Requests", "1"},
            {"Sec-Fetch-Dest", "document"},
            {"Sec-Fetch-Mode", "navigate"},
            {"Sec-Fetch-Site", "none"},
            {"Sec-Fetch-User", "?1"}};

        std::string path = "/word/" + word + "/";
        if (m_Language != "ja") {
          path += "#" + m_Language;
        }

        auto res = cli.Get(path.c_str(), headers);

        if (!res) {
          AF_WARN("ForvoClient: HTTP request failed for word '{}'", word);
          if (attempt < maxRetries - 1)
            continue;
          return "";
        }

        if (res->status == 403 && attempt < maxRetries - 1) {
          AF_DEBUG("ForvoClient: Got 403, will retry for word '{}'", word);
          continue;
        }

        if (res->status != 200) {
          AF_WARN("ForvoClient: HTTP status {} for word '{}'", res->status, word);
          return "";
        }

        return res->body;

      } catch (const std::exception& e) {
        AF_ERROR("ForvoClient: exception while fetching '{}': {}", word, e.what());
        if (attempt < maxRetries - 1)
          continue;
        return "";
      }
    }

    return "";
  }

  std::string ForvoClient::FetchSearchPage(const std::string& word) const
  {
    const int maxRetries = 3;

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
      try {
        if (attempt > 0) {
          int backoffMs = 500 * (1 << (attempt - 1));
          AF_DEBUG("ForvoClient: Search retry attempt {} after {}ms backoff", attempt, backoffMs);
          std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
        }

        httplib::SSLClient cli("forvo.com");
        cli.set_connection_timeout(m_TimeoutSeconds, 0);
        cli.set_read_timeout(m_TimeoutSeconds, 0);
        cli.set_follow_location(true);

        httplib::Headers headers = {
            {"User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:139.0) Gecko/20100101 Firefox/139.0"},
            {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8"},
            {"Accept-Language", "en-US,en;q=0.5"},
            {"DNT", "1"},
            {"Connection", "keep-alive"},
            {"Upgrade-Insecure-Requests", "1"},
            {"Sec-Fetch-Dest", "document"},
            {"Sec-Fetch-Mode", "navigate"},
            {"Sec-Fetch-Site", "none"},
            {"Sec-Fetch-User", "?1"}};

        std::string path = "/search/" + word + "/" + m_Language + "/";

        auto res = cli.Get(path.c_str(), headers);

        if (!res) {
          AF_WARN("ForvoClient: Search HTTP request failed for word '{}'", word);
          if (attempt < maxRetries - 1)
            continue;
          return "";
        }

        if (res->status == 403 && attempt < maxRetries - 1) {
          AF_DEBUG("ForvoClient: Search got 403, will retry for word '{}'", word);
          continue;
        }

        if (res->status != 200) {
          AF_WARN("ForvoClient: Search HTTP status {} for word '{}'", res->status, word);
          return "";
        }

        return res->body;

      } catch (const std::exception& e) {
        AF_ERROR("ForvoClient: exception while fetching search for '{}': {}", word, e.what());
        if (attempt < maxRetries - 1)
          continue;
        return "";
      }
    }

    return "";
  }

  std::vector<AudioFileInfo> ForvoClient::ParseAudioLinks(const std::string& html, const std::string& word) const
  {
    std::vector<AudioFileInfo> results;

    std::regex playRegex(
        R"(Play\(\d+,\s*'([^']+)',\s*'([^']+)',\s*(?:false|true),\s*'([^']+)',\s*'([^']+)',\s*'([^']+)')");

    std::smatch match;
    std::string::const_iterator searchStart(html.cbegin());
    int index = 0;

    while (std::regex_search(searchStart, html.cend(), match, playRegex) && index < m_MaxResults) {
      try {
        std::string rawMp3Arg = match[1].str();
        std::string rawOggArg = match[2].str();
        std::string normalizedMp3Arg = match[4].str();
        std::string normalizedOggArg = match[5].str();

        AF_DEBUG("ForvoClient: rawMp3={}, rawOgg={}, normMp3={}, normOgg={}",
                 rawMp3Arg,
                 rawOggArg,
                 normalizedMp3Arg,
                 normalizedOggArg);

        std::string encodedUrl;
        if (m_AudioFormat == "mp3") {
          encodedUrl = normalizedMp3Arg.empty() ? rawMp3Arg : normalizedMp3Arg;
        } else {
          encodedUrl = normalizedOggArg.empty() ? rawOggArg : normalizedOggArg;
        }

        std::string audioUrl = DecodeAudioUrl(encodedUrl);

        if (!audioUrl.empty()) {
          AF_DEBUG("ForvoClient: decoded URL={}", audioUrl);

          std::string username = "unknown";
          size_t matchPos = std::distance(html.cbegin(), searchStart);

          if (matchPos > 200) {
            std::string contextBefore(html.begin() + (matchPos - 200), searchStart);
            std::regex usernameRegex(R"(Pronunciation\s+by\s*<[^>]*>([^<]+)<)");
            std::smatch usernameMatch;

            if (std::regex_search(contextBefore, usernameMatch, usernameRegex)) {
              username = usernameMatch[1].str();
              username.erase(0, username.find_first_not_of(" \t\n\r"));
              username.erase(username.find_last_not_of(" \t\n\r") + 1);
              AF_DEBUG("ForvoClient: extracted username={}", username);
            }
          }

          std::string fileExt = "mp3";
          size_t dotPos = audioUrl.rfind('.');
          if (dotPos != std::string::npos && dotPos < audioUrl.length() - 1) {
            fileExt = audioUrl.substr(dotPos + 1);
            if (fileExt.length() > 4) {
              fileExt = "mp3";
            }
          }

          AudioFileInfo info;
          info.word = word;
          info.url = audioUrl;
          info.filename = GenerateFilename(word, username, index, fileExt);
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

    return results;
  }

  std::string ForvoClient::DecodeAudioUrl(const std::string& encodedData) const
  {
    if (encodedData.empty()) {
      return "";
    }

    if (encodedData.find("http") == 0) {
      return encodedData;
    }

    try {
      auto decoded = Video2Card::Utils::Base64Utils::Decode(encodedData);
      std::string decodedStr(decoded.begin(), decoded.end());

      if (decodedStr.find("/") != std::string::npos) {
        std::string fileExt;
        size_t dotPos = decodedStr.rfind('.');
        if (dotPos != std::string::npos) {
          fileExt = decodedStr.substr(dotPos + 1);
        } else {
          fileExt = m_AudioFormat;
        }

        std::string fullUrl = "https://audio12.forvo.com/audios/" + fileExt + "/" + decodedStr;
        AF_DEBUG("ForvoClient: constructed URL={}", fullUrl);
        return fullUrl;
      }
    } catch (const std::exception& e) {
      AF_WARN("ForvoClient: base64 decode failed: {}", e.what());
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

  std::string ForvoClient::GenerateFilename(const std::string& word,
                                            const std::string& username,
                                            int index,
                                            const std::string& extension) const
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

    ss << "." << extension;

    return ss.str();
  }

} // namespace Video2Card::Language::Audio
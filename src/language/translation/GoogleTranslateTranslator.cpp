#include "GoogleTranslateTranslator.h"

#include <exception>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "core/Logger.h"

namespace Video2Card::Language::Translation
{

  GoogleTranslateTranslator::GoogleTranslateTranslator(std::string sourceLang,
                                                       std::string targetLang,
                                                       int timeoutSeconds)
      : m_SourceLang(std::move(sourceLang))
      , m_TargetLang(std::move(targetLang))
      , m_TimeoutSeconds(timeoutSeconds)
  {}

  std::string GoogleTranslateTranslator::Translate(const std::string& text)
  {
    AF_DEBUG("GoogleTranslateTranslator::Translate called with text: '{}'", text);

    if (text.empty()) {
      AF_WARN("GoogleTranslateTranslator: Empty text provided");
      return "";
    }

    try {
      AF_DEBUG("GoogleTranslateTranslator: Connecting to translate.google.com");
      httplib::Client cli("https://translate.google.com");
      cli.set_connection_timeout(m_TimeoutSeconds, 0);
      cli.set_read_timeout(m_TimeoutSeconds, 0);
      cli.set_follow_location(true);

      std::string encodedText = text;
      std::string encoded;
      for (unsigned char c : encodedText) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
          encoded += c;
        } else {
          char hex[4];
          snprintf(hex, sizeof(hex), "%%%02X", c);
          encoded += hex;
        }
      }

      std::string path = "/m?sl=" + m_SourceLang + "&tl=" + m_TargetLang + "&q=" + encoded;

      AF_DEBUG("GoogleTranslateTranslator: Sending GET request to path: {}", path);
      auto res = cli.Get(path.c_str());

      if (!res) {
        AF_ERROR("GoogleTranslateTranslator: Request failed - {}", httplib::to_string(res.error()));
        return "";
      }

      if (res->status != 200) {
        AF_ERROR("GoogleTranslateTranslator: HTTP status {} received", res->status);
        AF_DEBUG("GoogleTranslateTranslator: Response body: {}", res->body);
        return "";
      }

      AF_DEBUG("GoogleTranslateTranslator: Received response, parsing HTML");

      std::string body = res->body;

      size_t resultDivPos = body.find("class=\"result-container\"");
      if (resultDivPos == std::string::npos) {
        AF_ERROR("GoogleTranslateTranslator: Could not find result container in HTML response");
        AF_DEBUG("GoogleTranslateTranslator: Response body preview (first 500 chars): {}",
                 body.substr(0, std::min(size_t(500), body.length())));
        return "";
      }

      size_t startPos = body.find(">", resultDivPos);
      if (startPos == std::string::npos) {
        AF_ERROR("GoogleTranslateTranslator: Could not find start position of result");
        return "";
      }
      startPos++;

      size_t endPos = body.find("</div>", startPos);
      if (endPos == std::string::npos) {
        AF_ERROR("GoogleTranslateTranslator: Could not find end position of result");
        return "";
      }

      std::string translation = body.substr(startPos, endPos - startPos);

      while (translation.find("<") != std::string::npos) {
        size_t tagStart = translation.find("<");
        size_t tagEnd = translation.find(">", tagStart);
        if (tagEnd != std::string::npos) {
          translation.erase(tagStart, tagEnd - tagStart + 1);
        } else {
          break;
        }
      }

      AF_INFO("GoogleTranslateTranslator: Successfully translated '{}' -> '{}'", text, translation);
      return translation;
    } catch (const std::exception& e) {
      AF_ERROR("GoogleTranslateTranslator: Exception during translation - {}", e.what());
      return "";
    }
  }

  bool GoogleTranslateTranslator::IsAvailable() const
  {
    try {
      AF_DEBUG("GoogleTranslateTranslator::IsAvailable - Checking connectivity to translate.google.com");
      httplib::Client cli("https://translate.google.com");
      cli.set_connection_timeout(3, 0);
      cli.set_read_timeout(3, 0);

      auto res = cli.Head("/");
      bool available = res && (res->status == 200 || res->status == 301 || res->status == 302);

      if (available) {
        AF_INFO("GoogleTranslateTranslator: Service is available (status: {})", res->status);
      } else {
        AF_WARN("GoogleTranslateTranslator: Service is NOT available");
      }

      return available;
    } catch (const std::exception& e) {
      AF_ERROR("GoogleTranslateTranslator::IsAvailable - Exception: {}", e.what());
      return false;
    } catch (...) {
      AF_ERROR("GoogleTranslateTranslator::IsAvailable - Unknown exception");
      return false;
    }
  }

} // namespace Video2Card::Language::Translation
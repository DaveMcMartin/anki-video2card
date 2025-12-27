#include "DeepLTranslator.h"

#include <exception>
#include <httplib.h>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "core/Logger.h"

namespace Video2Card::Language::Translation
{

  DeepLTranslator::DeepLTranslator(std::string apiKey, bool useFreeAPI, int timeoutSeconds) noexcept
      : m_ApiKey(std::move(apiKey))
      , m_UseFreeAPI(useFreeAPI)
      , m_TimeoutSeconds(timeoutSeconds)
      , m_SourceLang("JA")
      , m_TargetLang("EN-US")
      , m_Formality("default")
  {
    if (m_ApiKey.empty()) {
      AF_WARN("DeepL API key not configured - translation will be disabled");
    } else {
      AF_INFO("DeepL translator initialized (using {} API)", m_UseFreeAPI ? "free" : "pro");
    }
  }

  std::string DeepLTranslator::Translate(const std::string& text)
  {
    if (text.empty()) {
      return "";
    }

    // If API key is not configured, return original text
    if (m_ApiKey.empty()) {
      AF_DEBUG("DeepL API not configured - returning original text");
      return text;
    }

    try {
      // Determine host based on API tier
      std::string host = m_UseFreeAPI ? "api-free.deepl.com" : "api.deepl.com";

      httplib::SSLClient cli(host);
      cli.set_connection_timeout(m_TimeoutSeconds, 0);
      cli.set_read_timeout(m_TimeoutSeconds, 0);
      cli.set_write_timeout(m_TimeoutSeconds, 0);

      // Build request body
      std::stringstream body;
      body << "auth_key=" << UrlEncode(m_ApiKey) << "&text=" << UrlEncode(text)
           << "&source_lang=" << UrlEncode(m_SourceLang) << "&target_lang=" << UrlEncode(m_TargetLang);

      if (m_Formality != "default") {
        body << "&formality=" << UrlEncode(m_Formality);
      }

      // Set headers
      httplib::Headers headers = {{"Content-Type", "application/x-www-form-urlencoded"}};

      AF_DEBUG("Sending translation request to DeepL for text: {}", text.substr(0, 50));

      auto res = cli.Post("/v2/translate", headers, body.str(), "application/x-www-form-urlencoded");

      if (!res) {
        AF_WARN("DeepL API request failed: no response - returning original text");
        return text;
      }

      if (res->status != 200) {
        AF_WARN("DeepL API returned status {} - returning original text", res->status);
        return text;
      }

      std::string translation = ParseTranslationResponse(res->body);
      AF_DEBUG("Translation received: {}", translation);

      return translation;

    } catch (const std::exception& e) {
      AF_WARN("DeepL translation failed: {} - returning original text", e.what());
      return text;
    }
  }

  bool DeepLTranslator::IsAvailable() const
  {
    // Not available if API key is not configured
    if (m_ApiKey.empty()) {
      return false;
    }

    try {
      std::string host = m_UseFreeAPI ? "api-free.deepl.com" : "api.deepl.com";

      httplib::SSLClient cli(host);
      cli.set_connection_timeout(2, 0);

      httplib::Headers headers = {{"Authorization", "DeepL-Auth-Key " + m_ApiKey}};

      auto res = cli.Get("/v2/usage", headers);

      return res && res->status == 200;
    } catch (...) {
      return false;
    }
  }

  bool DeepLTranslator::IsConfigured() const noexcept
  {
    return !m_ApiKey.empty();
  }

  void DeepLTranslator::SetSourceLanguage(const std::string& lang) noexcept
  {
    m_SourceLang = lang;
  }

  void DeepLTranslator::SetTargetLanguage(const std::string& lang) noexcept
  {
    m_TargetLang = lang;
  }

  void DeepLTranslator::SetFormality(const std::string& formality) noexcept
  {
    if (formality == "default" || formality == "more" || formality == "less") {
      m_Formality = formality;
    } else {
      AF_WARN("Invalid formality level: {}. Using 'default'", formality);
      m_Formality = "default";
    }
  }

  std::string DeepLTranslator::GetApiEndpoint() const
  {
    if (m_UseFreeAPI) {
      return "https://api-free.deepl.com/v2/translate";
    } else {
      return "https://api.deepl.com/v2/translate";
    }
  }

  std::string DeepLTranslator::BuildRequestUrl(const std::string& text) const
  {
    std::stringstream url;
    url << GetApiEndpoint() << "?auth_key=" << UrlEncode(m_ApiKey) << "&text=" << UrlEncode(text)
        << "&source_lang=" << UrlEncode(m_SourceLang) << "&target_lang=" << UrlEncode(m_TargetLang);

    if (m_Formality != "default") {
      url << "&formality=" << UrlEncode(m_Formality);
    }

    return url.str();
  }

  std::string DeepLTranslator::ParseTranslationResponse(const std::string& json)
  {
    try {
      auto parsed = nlohmann::json::parse(json);

      if (!parsed.contains("translations") || !parsed["translations"].is_array()) {
        throw std::runtime_error("Invalid DeepL response format: missing translations array");
      }

      auto& translations = parsed["translations"];
      if (translations.empty()) {
        throw std::runtime_error("DeepL returned empty translations array");
      }

      auto& firstTranslation = translations[0];
      if (!firstTranslation.contains("text")) {
        throw std::runtime_error("DeepL translation missing 'text' field");
      }

      return firstTranslation["text"].get<std::string>();

    } catch (const nlohmann::json::exception& e) {
      throw std::runtime_error(std::string("Failed to parse DeepL response: ") + e.what());
    }
  }

  std::string DeepLTranslator::UrlEncode(const std::string& value)
  {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
      // Keep alphanumeric and other accepted characters intact
      if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
        escaped << c;
        continue;
      }

      // Any other characters are percent-encoded
      escaped << std::uppercase;
      escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
      escaped << std::nouppercase;
    }

    return escaped.str();
  }

} // namespace Video2Card::Language::Translation
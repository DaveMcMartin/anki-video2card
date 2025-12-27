#include "LocalAnalyzerAdapter.h"

#include "core/Logger.h"
#include "language/ILanguage.h"

namespace Video2Card::AI
{

  LocalAnalyzerAdapter::LocalAnalyzerAdapter(std::shared_ptr<ILocalAnalyzer> analyzer)
      : m_Analyzer(std::move(analyzer))
  {
    if (!m_Analyzer) {
      throw std::invalid_argument("Analyzer cannot be null");
    }
    AF_INFO("LocalAnalyzerAdapter initialized with: {}", m_Analyzer->GetName());
  }

  std::string LocalAnalyzerAdapter::GetName() const
  {
    return "Local Analyzer";
  }

  std::string LocalAnalyzerAdapter::GetId() const
  {
    return "local-analyzer";
  }

  bool LocalAnalyzerAdapter::RenderConfigurationUI()
  {
    // Local analyzer requires no configuration
    return false;
  }

  void LocalAnalyzerAdapter::LoadConfig(const nlohmann::json& json)
  {
    // No configuration needed
    (void) json;
  }

  nlohmann::json LocalAnalyzerAdapter::SaveConfig() const
  {
    return nlohmann::json::object();
  }

  void LocalAnalyzerAdapter::LoadRemoteModels()
  {
    // No remote models needed
  }

  nlohmann::json LocalAnalyzerAdapter::AnalyzeSentence(const std::string& sentence,
                                                       const std::string& targetWord,
                                                       Language::ILanguage* language)
  {
    (void) language; // Not used by local analyzer

    if (!m_Analyzer) {
      nlohmann::json error;
      error["error"] = "Analyzer not initialized";
      return error;
    }

    try {
      AF_INFO("Analyzing sentence with local analyzer: {}", sentence);

      auto result = m_Analyzer->AnalyzeSentence(sentence, targetWord);

      if (!result.contains("error")) {
        AF_DEBUG("Analysis successful");
      } else {
        AF_WARN("Analysis failed: {}", result["error"].dump());
      }

      return result;
    } catch (const std::exception& e) {
      AF_ERROR("Exception during local analysis: {}", e.what());
      nlohmann::json error;
      error["error"] = std::string("Analysis failed: ") + e.what();
      return error;
    }
  }

} // namespace Video2Card::AI
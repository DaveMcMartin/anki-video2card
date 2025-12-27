#pragma once

#include <memory>
#include <nlohmann/json.hpp>

#include "ITextAIProvider.h"
#include "language/analyzer/ILocalAnalyzer.h"

namespace Video2Card::AI
{

  using Language::Analyzer::ILocalAnalyzer;

  /**
 * Adapter that wraps a local analyzer (Mecab-based) to implement
 * the ITextAIProvider interface. This allows the local analyzer to
 * be used as a drop-in replacement for AI providers.
 */
  class LocalAnalyzerAdapter : public ITextAIProvider
  {
public:

    /**
   * Create an adapter wrapping a local analyzer.
   * @param analyzer The local analyzer to wrap
   */
    explicit LocalAnalyzerAdapter(std::shared_ptr<ILocalAnalyzer> analyzer);

    ~LocalAnalyzerAdapter() override = default;

    /**
   * Get the name of this provider.
   * @return "Local Analyzer"
   */
    std::string GetName() const override;

    /**
   * Get the identifier of this provider.
   * @return "local-analyzer"
   */
    std::string GetId() const override;

    /**
   * Render configuration UI for this provider.
   * Local analyzer has no configuration.
   * @return false (no configuration UI needed)
   */
    bool RenderConfigurationUI() override;

    /**
   * Load configuration from JSON (no-op for local analyzer).
   * @param json The configuration JSON (ignored)
   */
    void LoadConfig(const nlohmann::json& json) override;

    /**
   * Save configuration to JSON (returns empty for local analyzer).
   * @return Empty JSON object
   */
    nlohmann::json SaveConfig() const override;

    /**
   * Load remote models (no-op for local analyzer).
   */
    void LoadRemoteModels() override;

    /**
   * Analyze a sentence using the local analyzer.
   * @param sentence The sentence to analyze
   * @param targetWord The target word to focus on
   * @param language The language configuration
   * @return JSON with analysis results
   */
    nlohmann::json
    AnalyzeSentence(const std::string& sentence, const std::string& targetWord, Language::ILanguage* language) override;

private:

    std::shared_ptr<ILocalAnalyzer> m_Analyzer;
  };

} // namespace Video2Card::AI
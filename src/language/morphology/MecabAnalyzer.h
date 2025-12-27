#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IMorphologicalAnalyzer.h"

// Forward declare Mecab types to avoid including mecab.h in headers
struct mecab_t;
struct mecab_node_t;

namespace Video2Card::Language::Morphology
{

  /**
 * Mecab-based morphological analyzer for Japanese text.
 * Uses the Mecab morphological analyzer to parse Japanese text
 * into tokens with dictionary forms, readings, and POS information.
 */
  class MecabAnalyzer final : public IMorphologicalAnalyzer
  {
public:

    /**
   * Create a MecabAnalyzer instance.
   * @param dictionaryPath Optional path to Mecab dictionary. If empty, uses default.
   * @throws std::runtime_error if Mecab initialization fails
   */
    explicit MecabAnalyzer(const std::string& dictionaryPath = "");

    ~MecabAnalyzer() override;

    // Delete copy operations
    MecabAnalyzer(const MecabAnalyzer&) = delete;
    MecabAnalyzer& operator=(const MecabAnalyzer&) = delete;

    // Allow move operations
    MecabAnalyzer(MecabAnalyzer&& other) noexcept;
    MecabAnalyzer& operator=(MecabAnalyzer&& other) noexcept;

    /**
   * Analyze Japanese text and return morphological tokens.
   * @param text The Japanese text to analyze
   * @return List of MecabToken objects
   * @throws std::runtime_error if analysis fails
   */
    [[nodiscard]] MecabTokenList Analyze(const std::string& text) override;

    /**
   * Get the dictionary form of a word.
   * @param surface The surface form
   * @return Dictionary form, or empty if not found
   */
    [[nodiscard]] std::string GetDictionaryForm(const std::string& surface) override;

    /**
   * Get the reading of a word in katakana.
   * @param surface The surface form
   * @return Katakana reading, or empty if not found
   */
    [[nodiscard]] std::string GetReading(const std::string& surface) override;

    /**
   * Check if Mecab is properly initialized.
   * @return true if Mecab is ready to use
   */
    [[nodiscard]] bool IsInitialized() const;

private:

    /**
   * Parse a Mecab node and convert to MecabToken.
   * @param node The Mecab node to parse
   * @return MecabToken with extracted information
   */
    MecabToken ParseNode(mecab_node_t* node) const;

    /**
   * Parse a Mecab output line and convert to MecabToken.
   * @param line A line from Mecab output (surface\tfeatures)
   * @return MecabToken with extracted information
   */
    static MecabToken ParseMecabNode(const std::string& line);

    /**
   * Split CSV-formatted Mecab output (feature column).
   * @param features The CSV string from Mecab
   * @return Vector of parsed features
   */
    static std::vector<std::string> SplitFeatures(const std::string& features);

    mecab_t* m_Mecab;
    bool m_IsInitialized;
  };

} // namespace Video2Card::Language::Morphology
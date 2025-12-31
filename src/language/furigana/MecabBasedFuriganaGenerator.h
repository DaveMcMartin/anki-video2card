#pragma once

#include <memory>
#include <string>

#include "IFuriganaGenerator.h"
#include "language/morphology/IMorphologicalAnalyzer.h"

namespace Video2Card::Language::Furigana
{

  class MecabBasedFuriganaGenerator : public IFuriganaGenerator
  {
public:

    explicit MecabBasedFuriganaGenerator(std::shared_ptr<Morphology::IMorphologicalAnalyzer> analyzer);

    ~MecabBasedFuriganaGenerator() override = default;

    [[nodiscard]] std::string Generate(const std::string& text) override;

    [[nodiscard]] std::string GenerateForWord(const std::string& word) override;

private:

    std::shared_ptr<Morphology::IMorphologicalAnalyzer> m_Analyzer;

    std::string FormatFurigana(const std::string& word, const std::string& reading);

    std::string FormatFuriganaAdvanced(const std::string& word, const std::string& reading);

    static bool HasKanji(const std::string& text);
  };

} // namespace Video2Card::Language::Furigana
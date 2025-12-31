#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ITranslator.h"

namespace ctranslate2
{
  class Translator;
}

namespace Video2Card::Language::Translation
{

  class CTranslate2Translator : public ITranslator
  {
public:

    explicit CTranslate2Translator(const std::string& modelPath);

    ~CTranslate2Translator() override;

    CTranslate2Translator(const CTranslate2Translator&) = delete;
    CTranslate2Translator& operator=(const CTranslate2Translator&) = delete;
    CTranslate2Translator(CTranslate2Translator&&) noexcept = default;
    CTranslate2Translator& operator=(CTranslate2Translator&&) noexcept = default;

    [[nodiscard]] std::string Translate(const std::string& text) override;

    [[nodiscard]] bool IsAvailable() const override;

    void SetDevice(const std::string& device);

    void SetBeamSize(size_t beamSize);

private:

    [[nodiscard]] std::vector<std::string> Tokenize(const std::string& text) const;

    [[nodiscard]] std::string Detokenize(const std::vector<std::string>& tokens) const;

    std::unique_ptr<ctranslate2::Translator> m_Translator;
    std::string m_Device;
    size_t m_BeamSize;
    bool m_IsAvailable;
  };

} // namespace Video2Card::Language::Translation

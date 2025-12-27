#include "MecabAnalyzer.h"

#include <mecab.h>
#include <sstream>
#include <stdexcept>

#include "core/Logger.h"

namespace Video2Card::Language::Morphology
{

  MecabAnalyzer::MecabAnalyzer(const std::string& dictionaryPath)
      : m_Mecab(nullptr)
      , m_IsInitialized(false)
  {
    // Create Mecab instance
    // mecab_new requires argc and argv
    int argc = 1;
    const char* argv[] = {"mecab", "-d", "", nullptr};

    if (!dictionaryPath.empty()) {
      argc = 3;
      argv[2] = dictionaryPath.c_str();
    }

    m_Mecab = mecab_new(argc, const_cast<char**>(argv));

    if (!m_Mecab) {
      const char* error = mecab_strerror(nullptr);
      AF_ERROR("Failed to initialize Mecab: {}", error ? error : "Unknown error");
      throw std::runtime_error("Failed to initialize Mecab morphological analyzer");
    }

    m_IsInitialized = true;
    AF_INFO("Mecab morphological analyzer initialized successfully");
  }

  MecabAnalyzer::~MecabAnalyzer()
  {
    if (m_Mecab) {
      mecab_destroy(m_Mecab);
      m_Mecab = nullptr;
    }
    m_IsInitialized = false;
  }

  MecabAnalyzer::MecabAnalyzer(MecabAnalyzer&& other) noexcept
      : m_Mecab(other.m_Mecab)
      , m_IsInitialized(other.m_IsInitialized)
  {
    other.m_Mecab = nullptr;
    other.m_IsInitialized = false;
  }

  MecabAnalyzer& MecabAnalyzer::operator=(MecabAnalyzer&& other) noexcept
  {
    if (this != &other) {
      if (m_Mecab) {
        mecab_destroy(m_Mecab);
      }
      m_Mecab = other.m_Mecab;
      m_IsInitialized = other.m_IsInitialized;

      other.m_Mecab = nullptr;
      other.m_IsInitialized = false;
    }
    return *this;
  }

  MecabTokenList MecabAnalyzer::Analyze(const std::string& text)
  {
    if (!m_IsInitialized || !m_Mecab) {
      AF_ERROR("Mecab is not initialized");
      throw std::runtime_error("Mecab analyzer is not initialized");
    }

    MecabTokenList tokens;

    if (text.empty()) {
      return tokens;
    }

    // Use sparse_tostr for simple string output
    const char* result = mecab_sparse_tostr(m_Mecab, text.c_str());

    if (!result) {
      AF_ERROR("Mecab analysis failed");
      throw std::runtime_error("Mecab morphological analysis failed");
    }

    // Parse the result line by line
    std::istringstream stream(result);
    std::string line;

    while (std::getline(stream, line)) {
      // Skip empty lines and EOS marker
      if (line.empty() || line == "EOS") {
        continue;
      }

      try {
        tokens.push_back(ParseMecabNode(line));
      } catch (const std::exception& e) {
        AF_WARN("Failed to parse Mecab node: {}", e.what());
        continue;
      }
    }

    return tokens;
  }

  std::string MecabAnalyzer::GetDictionaryForm(const std::string& surface)
  {
    if (!m_IsInitialized || !m_Mecab) {
      return "";
    }

    try {
      auto tokens = Analyze(surface);
      if (!tokens.empty()) {
        return tokens[0].headword;
      }
    } catch (const std::exception& e) {
      AF_WARN("Error getting dictionary form for '{}': {}", surface, e.what());
    }

    return "";
  }

  std::string MecabAnalyzer::GetReading(const std::string& surface)
  {
    if (!m_IsInitialized || !m_Mecab) {
      return "";
    }

    try {
      auto tokens = Analyze(surface);
      if (!tokens.empty()) {
        return tokens[0].katakanaReading;
      }
    } catch (const std::exception& e) {
      AF_WARN("Error getting reading for '{}': {}", surface, e.what());
    }

    return "";
  }

  bool MecabAnalyzer::IsInitialized() const
  {
    return m_IsInitialized && m_Mecab != nullptr;
  }

  std::vector<std::string> MecabAnalyzer::SplitFeatures(const std::string& features)
  {
    std::vector<std::string> result;
    std::stringstream ss(features);
    std::string item;

    while (std::getline(ss, item, ',')) {
      result.push_back(item);
    }

    return result;
  }

  MecabToken MecabAnalyzer::ParseNode(mecab_node_t* node) const
  {
    // This method is here for potential future use with the node API
    // Currently, we use ParseMecabNode with string output instead
    MecabToken token;
    return token;
  }

  MecabToken MecabAnalyzer::ParseMecabNode(const std::string& line)
  {
    // Mecab output format: surface\tfeature1,feature2,...
    // Features are: POS, POS subclass 1-3, inflection type, inflection form, base form, reading

    size_t tabPos = line.find('\t');
    if (tabPos == std::string::npos) {
      throw std::runtime_error("Invalid Mecab output format: missing tab");
    }

    std::string surface = line.substr(0, tabPos);
    std::string featureStr = line.substr(tabPos + 1);

    auto features = SplitFeatures(featureStr);

    // Standard Mecab output has at least 7-9 fields depending on dictionary
    if (features.empty()) {
      throw std::runtime_error("Invalid Mecab output format: no features");
    }

    MecabToken token;
    token.surface = surface;

    // Features layout (standard IPA dictionary):
    // 0: POS (品詞)
    // 1: POS subclass 1 (品詞細分類1)
    // 2: POS subclass 2 (品詞細分類2)
    // 3: POS subclass 3 (品詞細分類3)
    // 4: Inflection type (活用型)
    // 5: Inflection form (活用形)
    // 6: Base form/dictionary form (基本形)
    // 7: Reading in katakana (読み)
    // 8: Pronunciation in katakana (発音)

    token.partOfSpeech = features.size() > 0 ? features[0] : "";
    token.posSubclass1 = features.size() > 1 ? features[1] : "";
    token.posSubclass2 = features.size() > 2 ? features[2] : "";
    token.posSubclass3 = features.size() > 3 ? features[3] : "";
    token.inflectionType = features.size() > 4 ? features[4] : "";
    token.inflectionForm = features.size() > 5 ? features[5] : "";
    token.headword = features.size() > 6 ? features[6] : surface;
    token.katakanaReading = features.size() > 7 ? features[7] : "";

    // If any field is "*", replace with empty string or surface form as appropriate
    if (token.headword == "*") {
      token.headword = surface;
    }
    if (token.katakanaReading == "*") {
      token.katakanaReading = "";
    }

    return token;
  }

} // namespace Video2Card::Language::Morphology
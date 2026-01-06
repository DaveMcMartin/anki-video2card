#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Video2Card::API
{

  class AnkiConnectClient
  {
public:

    explicit AnkiConnectClient(std::string url);
    ~AnkiConnectClient() = default;

    void SetUrl(const std::string& url);

    bool Ping();
    std::vector<std::string> GetDeckNames();
    std::vector<std::string> GetModelNames();
    std::vector<std::string> GetModelFieldNames(const std::string& modelName);
    int64_t AddNote(const std::string& deckName,
                    const std::string& modelName,
                    const std::map<std::string, std::string>& fields,
                    const std::vector<std::string>& tags = {});
    std::vector<int64_t> FindNotes(const std::string& query);
    bool StoreMediaFile(const std::string& filename, const std::string& base64Data);
    bool GuiBrowse(int64_t noteId);

private:

    nlohmann::json Execute(const std::string& action, const nlohmann::json& params = nullptr);

    std::string m_Url;
  };

} // namespace Video2Card::API

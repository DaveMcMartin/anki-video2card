#include "api/AnkiConnectClient.h"

#include <httplib.h>
#include <iostream>

#include "core/Logger.h"

namespace Video2Card::API
{

  AnkiConnectClient::AnkiConnectClient(std::string url)
  {
    SetUrl(std::move(url));
  }

  void AnkiConnectClient::SetUrl(const std::string& url)
  {
    m_Url = url;
    size_t pos = m_Url.find("localhost");
    if (pos != std::string::npos) {
      m_Url.replace(pos, 9, "127.0.0.1");
    }
  }

  nlohmann::json AnkiConnectClient::Execute(const std::string& action, const nlohmann::json& params)
  {
    try {
      httplib::Client cli(m_Url);
      cli.set_connection_timeout(120);
      cli.set_read_timeout(120);

      nlohmann::json request;
      request["action"] = action;
      request["version"] = 6;
      if (!params.is_null()) {
        request["params"] = params;
      }

      auto res = cli.Post("/", request.dump(), "application/json");

      if (!res) {
        AF_ERROR("AnkiConnect Connection Error: {} ({})", httplib::to_string(res.error()), m_Url);
        return nullptr;
      }

      if (res->status != 200) {
        AF_ERROR("AnkiConnect HTTP Error: {}", res->status);
        return nullptr;
      }

      auto response = nlohmann::json::parse(res->body);
      if (!response["error"].is_null()) {
        AF_ERROR("AnkiConnect Error ({}): {}", action, response["error"].dump());
        return nullptr;
      }
      return response["result"];
    } catch (const std::exception& e) {
      AF_ERROR("AnkiConnect Exception: {}", e.what());
      return nullptr;
    }
  }

  bool AnkiConnectClient::Ping()
  {
    auto result = Execute("version");
    return !result.is_null();
  }

  std::vector<std::string> AnkiConnectClient::GetDeckNames()
  {
    std::vector<std::string> decks;
    auto result = Execute("deckNames");
    if (result.is_array()) {
      decks = result.get<std::vector<std::string>>();
    }
    return decks;
  }

  std::vector<std::string> AnkiConnectClient::GetModelNames()
  {
    std::vector<std::string> models;
    auto result = Execute("modelNames");
    if (result.is_array()) {
      models = result.get<std::vector<std::string>>();
    }
    return models;
  }

  std::vector<std::string> AnkiConnectClient::GetModelFieldNames(const std::string& modelName)
  {
    std::vector<std::string> fields;
    nlohmann::json params;
    params["modelName"] = modelName;

    auto result = Execute("modelFieldNames", params);
    if (result.is_array()) {
      fields = result.get<std::vector<std::string>>();
    }
    return fields;
  }

  int64_t AnkiConnectClient::AddNote(const std::string& deckName,
                                     const std::string& modelName,
                                     const std::map<std::string, std::string>& fields,
                                     const std::vector<std::string>& tags)
  {
    nlohmann::json note;
    note["deckName"] = deckName;
    note["modelName"] = modelName;

    nlohmann::json fieldsJson;
    for (const auto& [key, value] : fields) {
      fieldsJson[key] = value;
    }
    note["fields"] = fieldsJson;
    note["tags"] = tags;

    nlohmann::json params;
    params["note"] = note;

    auto result = Execute("addNote", params);
    if (result.is_number_integer()) {
      return result.get<int64_t>();
    }
    return 0;
  }

  std::vector<int64_t> AnkiConnectClient::FindNotes(const std::string& query)
  {
    std::vector<int64_t> noteIds;
    nlohmann::json params;
    params["query"] = query;

    auto result = Execute("findNotes", params);
    if (result.is_array()) {
      noteIds = result.get<std::vector<int64_t>>();
    }
    return noteIds;
  }

  bool AnkiConnectClient::StoreMediaFile(const std::string& filename, const std::string& base64Data)
  {
    nlohmann::json params;
    params["filename"] = filename;
    params["data"] = base64Data;

    auto result = Execute("storeMediaFile", params);
    return !result.is_null();
  }

  bool AnkiConnectClient::GuiBrowse(int64_t noteId)
  {
    nlohmann::json params;
    params["query"] = "nid:" + std::to_string(noteId);

    auto result = Execute("guiBrowse", params);
    return !result.is_null();
  }

} // namespace Video2Card::API

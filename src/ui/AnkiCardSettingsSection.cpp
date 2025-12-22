#include "ui/AnkiCardSettingsSection.h"

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <map>

#include "IconsFontAwesome6.h"
#include "api/AnkiConnectClient.h"
#include "config/ConfigManager.h"
#include "core/Logger.h"
#include "utils/Base64Utils.h"
#include "utils/ImageProcessor.h"

namespace Video2Card::UI
{

  AnkiCardSettingsSection::AnkiCardSettingsSection(SDL_Renderer* renderer,
                                                   API::AnkiConnectClient* ankiConnectClient,
                                                   Config::ConfigManager* configManager)
      : m_Renderer(renderer)
      , m_AnkiConnectClient(ankiConnectClient)
      , m_ConfigManager(configManager)
  {
    if (m_ConfigManager) {
      const auto& config = m_ConfigManager->GetConfig();
      m_NoteTypes = config.AnkiNoteTypes;
      m_Decks = config.AnkiDecks;

      if (!m_NoteTypes.empty()) {
        auto it = std::find(m_NoteTypes.begin(), m_NoteTypes.end(), config.LastNoteType);
        if (it != m_NoteTypes.end()) {
          m_SelectedNoteTypeIndex = (int) std::distance(m_NoteTypes.begin(), it);
        }
      }

      if (!m_Decks.empty()) {
        auto it = std::find(m_Decks.begin(), m_Decks.end(), config.LastDeck);
        if (it != m_Decks.end()) {
          m_SelectedDeckIndex = (int) std::distance(m_Decks.begin(), it);
        }
      }
    }
  }

  void AnkiCardSettingsSection::RefreshData()
  {
    if (!m_AnkiConnectClient)
      return;

    m_NoteTypes = m_AnkiConnectClient->GetModelNames();
    m_Decks = m_AnkiConnectClient->GetDeckNames();

    if (m_ConfigManager) {
      m_ConfigManager->GetConfig().AnkiNoteTypes = m_NoteTypes;
      m_ConfigManager->GetConfig().AnkiDecks = m_Decks;
      m_ConfigManager->Save();
    }

    m_SelectedNoteTypeIndex = 0;
    m_SelectedDeckIndex = 0;

    if (m_ConfigManager) {
      const auto& config = m_ConfigManager->GetConfig();

      auto itNote = std::find(m_NoteTypes.begin(), m_NoteTypes.end(), config.LastNoteType);
      if (itNote != m_NoteTypes.end()) {
        m_SelectedNoteTypeIndex = (int) std::distance(m_NoteTypes.begin(), itNote);
      }

      auto itDeck = std::find(m_Decks.begin(), m_Decks.end(), config.LastDeck);
      if (itDeck != m_Decks.end()) {
        m_SelectedDeckIndex = (int) std::distance(m_Decks.begin(), itDeck);
      }
    }

    m_Fields.clear();

    if (!m_NoteTypes.empty()) {
      std::string currentNoteType = m_NoteTypes[m_SelectedNoteTypeIndex];
      auto fieldNames = m_AnkiConnectClient->GetModelFieldNames(currentNoteType);
      for (const auto& name : fieldNames) {
        bool enabled = false;
        int toolIdx = 0;

        if (m_ConfigManager) {
          const auto& config = m_ConfigManager->GetConfig();
          if (config.FieldMappings.count(currentNoteType) && config.FieldMappings.at(currentNoteType).count(name)) {
            auto& pair = config.FieldMappings.at(currentNoteType).at(name);
            enabled = pair.first;
            toolIdx = pair.second;
          }
        }

        m_Fields.push_back(std::make_unique<CardField>(name));
        m_Fields.back()->SetToolEnabled(enabled);
        m_Fields.back()->SetSelectedToolIndex(toolIdx);
      }
    }
  }

  void AnkiCardSettingsSection::SetField(const std::string& name, const std::string& value)
  {
    for (auto& field : m_Fields) {
      if (field->GetName() == name) {
        AF_INFO("Setting field '{}' to '{}'", name, value);
        field->SetValue(value);
        return;
      }
    }
    AF_WARN("Field '{}' not found in current note type.", name);
  }

  void AnkiCardSettingsSection::SetFieldByTool(int toolIndex, const std::string& value)
  {
    for (auto& field : m_Fields) {
      if (field->IsToolEnabled() && field->GetSelectedToolIndex() == toolIndex) {
        AF_INFO("Auto-filling field '{}' with tool index {}", field->GetName(), toolIndex);
        field->SetValue(value);

        // Heuristic: if tool is text-based, ensure type is Text
        if (toolIndex < 7) // 0-6 are text/furigana/def
        {
          field->SetType(CardFieldType::Text);
        }
      }
    }
  }

  void AnkiCardSettingsSection::SetFieldByTool(int toolIndex,
                                               const std::vector<unsigned char>& data,
                                               const std::string& filename)
  {
    for (auto& field : m_Fields) {
      if (field->IsToolEnabled() && field->GetSelectedToolIndex() == toolIndex) {
        AF_INFO("Auto-filling field '{}' with tool index {} (binary)", field->GetName(), toolIndex);
        field->SetBinaryData(data, filename);

        // Heuristic: set type based on tool index
        if (toolIndex == 7) // Image
        {
          field->SetType(CardFieldType::Image);
        } else if (toolIndex == 8 || toolIndex == 9) // Audio
        {
          field->SetType(CardFieldType::Audio);
        }
      }
    }
  }

  AnkiCardSettingsSection::~AnkiCardSettingsSection() {}

  void AnkiCardSettingsSection::Render()
  {
    ImGui::Text("Note Type");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    const char* currentNoteType = m_NoteTypes.empty() ? "" : m_NoteTypes[m_SelectedNoteTypeIndex].c_str();
    if (ImGui::BeginCombo("##NoteType", currentNoteType)) {
      for (size_t i = 0; i < m_NoteTypes.size(); i++) {
        const bool is_selected = (m_SelectedNoteTypeIndex == (int) i);
        if (ImGui::Selectable(m_NoteTypes[i].c_str(), is_selected)) {
          m_SelectedNoteTypeIndex = (int) i;

          if (m_ConfigManager) {
            m_ConfigManager->GetConfig().LastNoteType = m_NoteTypes[i];
            m_ConfigManager->Save();
          }

          // Refresh fields based on note type
          m_Fields.clear();
          if (m_AnkiConnectClient) {
            std::string currentNoteType = m_NoteTypes[i];
            auto fieldNames = m_AnkiConnectClient->GetModelFieldNames(currentNoteType);
            for (const auto& name : fieldNames) {
              bool enabled = false;
              int toolIdx = 0;

              if (m_ConfigManager) {
                const auto& config = m_ConfigManager->GetConfig();
                if (config.FieldMappings.count(currentNoteType) && config.FieldMappings.at(currentNoteType).count(name))
                {
                  auto& pair = config.FieldMappings.at(currentNoteType).at(name);
                  enabled = pair.first;
                  toolIdx = pair.second;
                }
              }

              m_Fields.push_back(std::make_unique<CardField>(name));
              m_Fields.back()->SetToolEnabled(enabled);
              m_Fields.back()->SetSelectedToolIndex(toolIdx);
            }
          }
        }
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::Spacing();

    ImGui::Text("Deck");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    const char* currentDeck = m_Decks.empty() ? "" : m_Decks[m_SelectedDeckIndex].c_str();
    if (ImGui::BeginCombo("##Deck", currentDeck)) {
      for (size_t i = 0; i < m_Decks.size(); i++) {
        const bool is_selected = (m_SelectedDeckIndex == (int) i);
        if (ImGui::Selectable(m_Decks[i].c_str(), is_selected)) {
          m_SelectedDeckIndex = (int) i;
          if (m_ConfigManager) {
            m_ConfigManager->GetConfig().LastDeck = m_Decks[i];
            m_ConfigManager->Save();
          }
        }
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::Separator();
    ImGui::Spacing();

    for (auto& field : m_Fields) {
      bool wasEnabled = field->IsToolEnabled();
      int oldTool = field->GetSelectedToolIndex();

      field->Render(m_Renderer);

      if (wasEnabled != field->IsToolEnabled() || oldTool != field->GetSelectedToolIndex()) {
        if (m_ConfigManager && !m_NoteTypes.empty()) {
          std::string currentNoteType = m_NoteTypes[m_SelectedNoteTypeIndex];
          m_ConfigManager->GetConfig().FieldMappings[currentNoteType][field->GetName()].first = field->IsToolEnabled();
          m_ConfigManager->GetConfig().FieldMappings[currentNoteType][field->GetName()].second =
              field->GetSelectedToolIndex();
          m_ConfigManager->Save();
        }
      }

      ImGui::Spacing();
    }
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button(ICON_FA_TRASH " Clear", ImVec2(100, 0))) {
      for (auto& field : m_Fields) {
        field->SetValue("");
        field->SetBinaryData({}, "");
      }
    }

    ImGui::SameLine();

    if (m_LastCardId > 0) {
      if (ImGui::Button(ICON_FA_EYE " See Last", ImVec2(100, 0))) {
        if (m_AnkiConnectClient) {
          m_AnkiConnectClient->GuiBrowse(m_LastCardId);
        }
      }
      ImGui::SameLine();
    }

    float availWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - 100);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.59f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.69f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.49f, 0.10f, 1.0f));
    if (ImGui::Button(ICON_FA_PLUS " Add", ImVec2(100, 0))) {
      CheckDuplicatesAndAdd();
    }
    ImGui::PopStyleColor(3);

    RenderDuplicateModal();
  }

  void AnkiCardSettingsSection::CheckDuplicatesAndAdd()
  {
    if (!m_AnkiConnectClient || m_NoteTypes.empty() || m_Decks.empty())
      return;

    std::string deckName = m_Decks[m_SelectedDeckIndex];
    std::string modelName = m_NoteTypes[m_SelectedNoteTypeIndex];

    // Construct query to check for duplicates
    // We check for Sentence or Vocab Word
    std::string query = "deck:\"" + deckName + "\" note:\"" + modelName + "\" (";
    bool hasCriteria = false;

    for (const auto& field : m_Fields) {
      // Check if this field is mapped to Sentence (0) or Vocab Word (3)
      // Or if the field name itself suggests it (fallback)
      bool isSentence = (field->IsToolEnabled() && field->GetSelectedToolIndex() == 0) ||
                        field->GetName() == "Sentence" || field->GetName() == "Expression";
      bool isVocab = (field->IsToolEnabled() && field->GetSelectedToolIndex() == 3) ||
                     field->GetName() == "Target Word" || field->GetName() == "Vocab Word";

      if ((isSentence || isVocab) && !field->GetValue().empty()) {
        if (hasCriteria)
          query += " OR ";
        // Escape quotes in value
        std::string escapedValue = field->GetValue();
        // Simple escape for now
        size_t pos = 0;
        while ((pos = escapedValue.find("\"", pos)) != std::string::npos) {
          escapedValue.replace(pos, 1, "\\\"");
          pos += 2;
        }

        query += "\"" + field->GetName() + ":" + escapedValue + "\"";
        hasCriteria = true;
      }
    }
    query += ")";

    if (!hasCriteria) {
      // No fields to check, just add? Or warn?
      // Let's just add for now, or maybe show validation error if no fields are filled at all
      bool anyFieldFilled = false;
      for (const auto& field : m_Fields)
        if (!field->GetValue().empty())
          anyFieldFilled = true;

      if (anyFieldFilled)
        PerformAdd();

      return;
    }

    AF_INFO("Checking for duplicates with query: {}", query);
    auto notes = m_AnkiConnectClient->FindNotes(query);

    if (!notes.empty()) {
      m_DuplicateMessage =
          "Found " + std::to_string(notes.size()) + " duplicate note(s) in deck '" + deckName + "'.\nAdd anyway?";
      m_ShowDuplicateModal = true;
      m_OpenDuplicateModal = true;
    } else {
      PerformAdd();
    }
  }

  void AnkiCardSettingsSection::RenderDuplicateModal()
  {
    if (m_OpenDuplicateModal) {
      ImGui::OpenPopup("Duplicate Warning");
      m_OpenDuplicateModal = false;
    }

    if (ImGui::BeginPopupModal("Duplicate Warning", &m_ShowDuplicateModal, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("%s", m_DuplicateMessage.c_str());
      ImGui::Separator();

      if (ImGui::Button("Add Anyway", ImVec2(120, 0))) {
        PerformAdd();
        m_ShowDuplicateModal = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SetItemDefaultFocus();
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        m_ShowDuplicateModal = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }

  void AnkiCardSettingsSection::PerformAdd()
  {
    if (!m_AnkiConnectClient || m_NoteTypes.empty() || m_Decks.empty())
      return;

    std::string deckName = m_Decks[m_SelectedDeckIndex];
    std::string modelName = m_NoteTypes[m_SelectedNoteTypeIndex];
    std::map<std::string, std::string> fieldsMap;

    for (const auto& field : m_Fields) {
      std::string fieldValue = field->GetValue();
      const auto& binaryData = field->GetBinaryData();

      if (!binaryData.empty()) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::string uniqueFilename = std::to_string(timestamp) + "_" + fieldValue;

        // Process binary data based on field type
        std::vector<unsigned char> processedData = binaryData;

        if (field->GetType() == CardFieldType::Image) {
          // Compress image to WebP format, scaling to fit 320x320
          AF_INFO("Compressing image to WebP format (max 320x320)...");
          processedData = Utils::ImageProcessor::ScaleAndCompressToWebP(binaryData, 320, 320, 75);
          if (processedData.empty()) {
            AF_WARN("Failed to compress image, using original");
            processedData = binaryData;
          }
          // Update filename extension for WebP
          size_t dotPos = uniqueFilename.find_last_of(".");
          if (dotPos != std::string::npos) {
            uniqueFilename = uniqueFilename.substr(0, dotPos) + ".webp";
          } else {
            uniqueFilename += ".webp";
          }
          AF_INFO("Image compressed: {} bytes -> {} bytes", binaryData.size(), processedData.size());
        }

        std::string base64Data = Video2Card::Utils::Base64Utils::Encode(processedData);

        if (m_AnkiConnectClient->StoreMediaFile(uniqueFilename, base64Data)) {
          if (field->GetType() == CardFieldType::Image) {
            fieldValue = "<img src=\"" + uniqueFilename + "\">";
          } else if (field->GetType() == CardFieldType::Audio) {
            fieldValue = "[sound:" + uniqueFilename + "]";
          }
        } else {
          AF_ERROR("Failed to upload media file: {}", uniqueFilename);
          continue;
        }
      }

      if (!fieldValue.empty()) {
        fieldsMap[field->GetName()] = fieldValue;
      }
    }

    int64_t noteId = m_AnkiConnectClient->AddNote(deckName, modelName, fieldsMap, {"video2card"});
    if (noteId > 0) {
      m_LastCardId = noteId;
      AF_INFO("Note added successfully. Card ID: {}", noteId);
      if (m_OnStatusMessage)
        m_OnStatusMessage("Note added successfully.");

      for (auto& field : m_Fields) {
        field->SetValue("");
        field->SetBinaryData({}, "");
      }
    } else {
      AF_ERROR("Failed to add note.");
      if (m_OnStatusMessage)
        m_OnStatusMessage("Failed to add note.");
    }
  }

} // namespace Video2Card::UI

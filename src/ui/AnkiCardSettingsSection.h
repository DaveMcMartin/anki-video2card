#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ui/UIComponent.h"
#include "ui/fields/CardField.h"

struct SDL_Renderer;

namespace Video2Card::API
{
  class AnkiConnectClient;
}

namespace Video2Card::Config
{
  class ConfigManager;
}

namespace Video2Card::UI
{

  class AnkiCardSettingsSection : public UIComponent
  {
public:

    explicit AnkiCardSettingsSection(SDL_Renderer* renderer,
                                     API::AnkiConnectClient* ankiConnectClient,
                                     Config::ConfigManager* configManager);
    ~AnkiCardSettingsSection() override;

    void Render() override;
    void RefreshData();
    void SetField(const std::string& name, const std::string& value);
    void SetFieldByTool(int toolIndex, const std::string& value);
    void SetFieldByTool(int toolIndex, const std::vector<unsigned char>& data, const std::string& filename);

    void SetOnStatusMessageCallback(std::function<void(const std::string&)> callback) { m_OnStatusMessage = callback; }

private:

    void RenderDuplicateModal();
    void CheckDuplicatesAndAdd();
    void PerformAdd();

    // State
    int m_SelectedNoteTypeIndex = 0;
    int m_SelectedDeckIndex = 0;

    // Mock data for now, eventually populated from AnkiConnect
    std::vector<std::string> m_NoteTypes;
    std::vector<std::string> m_Decks;
    std::vector<std::unique_ptr<CardField>> m_Fields;

    // Duplicate Check State
    bool m_ShowDuplicateModal = false;
    bool m_OpenDuplicateModal = false;
    std::string m_DuplicateMessage;

    SDL_Renderer* m_Renderer;
    API::AnkiConnectClient* m_AnkiConnectClient;
    Config::ConfigManager* m_ConfigManager;

    std::function<void(const std::string&)> m_OnStatusMessage;

    int64_t m_LastCardId = 0;
  };

} // namespace Video2Card::UI

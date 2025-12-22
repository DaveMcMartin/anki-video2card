#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>
#include <vector>

#include "audio/AudioPlayer.h"
#include "core/FieldTypes.h"

struct SDL_Renderer;
struct SDL_Texture;

namespace Video2Card
{
  namespace UI
  {

    enum class CardFieldType
    {
      Text,
      Audio,
      Image
    };

    class CardField
    {
  public:

      CardField(const std::string& name);
      ~CardField();

      void Render(SDL_Renderer* renderer);

      // Getters
      const std::string& GetName() const { return m_Name; }
      const std::string& GetValue() const { return m_Value; }
      const std::vector<unsigned char>& GetBinaryData() const { return m_BinaryData; }
      CardFieldType GetType() const { return m_Type; }

      bool IsToolEnabled() const { return m_IsToolEnabled; }
      int GetSelectedToolIndex() const { return m_SelectedToolIndex; }

      // Setters
      void SetValue(const std::string& value);
      void SetBinaryData(const std::vector<unsigned char>& data, const std::string& filename);
      void SetType(CardFieldType type);

      // Auto-fill configuration
      void SetToolEnabled(bool enabled) { m_IsToolEnabled = enabled; }
      void SetSelectedToolIndex(int index) { m_SelectedToolIndex = index; }

  private:

      void RenderTypeSelector();
      void RenderTextContent();
      void RenderAudioContent();
      void RenderImageContent(SDL_Renderer* renderer);

      void ClearContent();

      // File loading helpers
      void LoadAudioFile();
      void LoadImageFile();

      std::string m_Name;
      CardFieldType m_Type = CardFieldType::Text;

      // Content
      std::string m_Value;                     // Text content or Filename
      std::vector<unsigned char> m_BinaryData; // Audio or Image data

      // Image Preview
      SDL_Texture* m_ImageTexture = nullptr;
      int m_ImageWidth = 0;
      int m_ImageHeight = 0;
      bool m_TextureNeedsUpdate = false;

      // Auto-fill State
      bool m_IsToolEnabled = false;
      int m_SelectedToolIndex = 0;

      std::unique_ptr<Audio::AudioPlayer> m_AudioPlayer;
    };

  } // namespace UI
} // namespace Video2Card

#include "Application.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_stdlib.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "ai/GoogleTextProvider.h"
#include "ai/ITextAIProvider.h"
#include "ai/XAiTextProvider.h"
#include "api/AnkiConnectClient.h"
#include "config/ConfigManager.h"
#include "core/Logger.h"
#include "core/sdl/SDLWrappers.h"
#include "utils/FileUtils.h"
#include "language/ILanguage.h"
#include "language/JapaneseLanguage.h"
#include "ui/AnkiCardSettingsSection.h"
#include "ui/ConfigurationSection.h"
#include "ui/StatusSection.h"
#include "ui/VideoSection.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "IconsFontAwesome6.h"

namespace Video2Card
{

  Application::Application(std::string title, int width, int height)
      : m_Title(std::move(title)), m_Width(width), m_Height(height), m_IsRunning(true), m_Window(nullptr),
        m_Renderer(nullptr)
  {}

  Application::~Application()
  {
    Shutdown();
  }

  void Application::Run()
  {
    if (!Initialize()) {
      return;
    }

    while (m_IsRunning) {
      HandleEvents();
      Update();
      Render();
    }
  }

  bool Application::Initialize()
  {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
      AF_ERROR("Error: SDL_Init(): {}", SDL_GetError());
      return false;
    }

    const char* base = SDL_GetBasePath();
    if (base) {
      m_BasePath = base;
      SDL_free((void*) base);
    } else {
      AF_ERROR("SDL_GetBasePath failed: {}", SDL_GetError());
    }

    auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN |
                                           SDL_WINDOW_HIGH_PIXEL_DENSITY);
    m_Window = SDL_CreateWindow(m_Title.c_str(), m_Width, m_Height, window_flags);
    if (m_Window == nullptr) {
      AF_ERROR("Error: SDL_CreateWindow(): {}", SDL_GetError());
      return false;
    }

    m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
    if (m_Renderer == nullptr) {
      AF_ERROR("Error: SDL_CreateRenderer(): {}", SDL_GetError());
      return false;
    }

    SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    {
      const std::string iconPath = m_BasePath + "assets/logo.png";
      int iconWidth{}, iconHeight{}, iconChannels{};
      unsigned char* iconData = stbi_load(iconPath.c_str(), &iconWidth, &iconHeight, &iconChannels, 4);
      if (iconData) {
        auto iconSurface = SDL::MakeSurfaceFrom(iconWidth, iconHeight, SDL_PIXELFORMAT_RGBA32, iconData, iconWidth * 4);

        if (iconSurface) {
          SDL_SetWindowIcon(m_Window, iconSurface.get());
        } else {
          AF_WARN("Failed to create icon surface: {}", SDL_GetError());
        }
        stbi_image_free(iconData);
      } else {
        AF_WARN("Failed to load {} for window icon.", iconPath);
      }
    }

    SDL_ShowWindow(m_Window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.3f;
    style.FrameRounding = 2.3f;
    style.ScrollbarRounding = 0;

    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.01f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.87f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.48f, 0.72f, 0.89f, 0.49f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(m_Window, m_Renderer);
    ImGui_ImplSDLRenderer3_Init(m_Renderer);

    std::string fontPath = m_BasePath + "assets/NotoSansJP-Regular.otf";
    float fontSize = 24.0f;
    ImFont* font =
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesJapanese());
    if (font == nullptr) {
      AF_WARN("Warning: Could not load font {}. Using default font.", fontPath);
    }

    float iconFontSize = fontSize * 2.0f / 3.0f;
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;

    std::string iconFontPath = m_BasePath + "assets/fa-solid-900.ttf";
    io.Fonts->AddFontFromFileTTF(iconFontPath.c_str(), iconFontSize, &icons_config, icons_ranges);

    m_ConfigManager = std::make_unique<Config::ConfigManager>(Utils::FileUtils::GetConfigPath());

    // Initialize language system
    m_Languages.push_back(std::make_unique<Language::JapaneseLanguage>());
    // Set active language from config
    std::string selectedLang = m_ConfigManager->GetConfig().SelectedLanguage;
    for (auto& lang : m_Languages) {
      if (lang->GetIdentifier() == selectedLang) {
        m_ActiveLanguage = lang.get();
        break;
      }
    }
    // Default to first language if not found
    if (!m_ActiveLanguage && !m_Languages.empty()) {
      m_ActiveLanguage = m_Languages[0].get();
    }

    // Initialize text AI providers
    m_TextAIProviders.push_back(std::make_unique<AI::GoogleTextProvider>());
    m_TextAIProviders.push_back(std::make_unique<AI::XAiTextProvider>());

    // Set active provider from config
    std::string selectedProvider = m_ConfigManager->GetConfig().TextProvider;
    for (auto& provider : m_TextAIProviders) {
      if (provider->GetId() == selectedProvider || provider->GetName() == selectedProvider) {
        m_ActiveTextAIProvider = provider.get();
        break;
      }
    }
    // Default to first provider if not found
    if (!m_ActiveTextAIProvider && !m_TextAIProviders.empty()) {
      m_ActiveTextAIProvider = m_TextAIProviders[0].get();
    }

    // Load provider configs
    for (auto& provider : m_TextAIProviders) {
      nlohmann::json providerConfig;
      if (provider->GetId() == "xai") {
        providerConfig["api_key"] = m_ConfigManager->GetConfig().TextApiKey;
        providerConfig["vision_model"] = m_ConfigManager->GetConfig().TextVisionModel;
        providerConfig["sentence_model"] = m_ConfigManager->GetConfig().TextSentenceModel;
        providerConfig["available_models"] = m_ConfigManager->GetConfig().TextAvailableModels;
      } else if (provider->GetId() == "google") {
        providerConfig["api_key"] = m_ConfigManager->GetConfig().GoogleApiKey;
        providerConfig["vision_model"] = m_ConfigManager->GetConfig().GoogleVisionModel;
        providerConfig["sentence_model"] = m_ConfigManager->GetConfig().GoogleSentenceModel;
        providerConfig["available_models"] = m_ConfigManager->GetConfig().GoogleAvailableModels;
      }
      provider->LoadConfig(providerConfig);
    }

    std::string ankiUrl = m_ConfigManager->GetConfig().AnkiConnectUrl;
    if (ankiUrl.empty())
      ankiUrl = "http://localhost:8765";
    m_AnkiConnectClient = std::make_unique<API::AnkiConnectClient>(ankiUrl);

    m_VideoSection = std::make_unique<UI::VideoSection>(m_Renderer, m_ConfigManager.get(), &m_Languages,
                                                        &m_ActiveLanguage);
    m_ConfigurationSection = std::make_unique<UI::ConfigurationSection>(m_AnkiConnectClient.get(),
                                                                        m_ConfigManager.get(),
                                                                        &m_TextAIProviders,
                                                                        &m_ActiveTextAIProvider,
                                                                        &m_Languages,
                                                                        &m_ActiveLanguage);
    m_AnkiCardSettingsSection =
        std::make_unique<UI::AnkiCardSettingsSection>(m_Renderer, m_AnkiConnectClient.get(), m_ConfigManager.get());
    m_StatusSection = std::make_unique<UI::StatusSection>();

    m_AnkiCardSettingsSection->SetOnStatusMessageCallback([this](const std::string& msg) {
      if (m_StatusSection)
        m_StatusSection->SetStatus(msg);
    });

    m_ConfigurationSection->SetOnConnectCallback([this]() {
      if (m_AnkiCardSettingsSection) {
        m_AnkiCardSettingsSection->RefreshData();
      }
      m_AnkiConnected.store(true);
      if (m_StatusSection)
        m_StatusSection->SetStatus("AnkiConnect: Connected");
    });

    m_VideoSection->SetOnExtractCallback([this]() { OnExtract(); });

    std::thread([this]() {
      if (m_AnkiConnectClient && m_AnkiConnectClient->Ping()) {
        m_AnkiConnected.store(true);
        if (m_StatusSection)
          m_StatusSection->SetStatus("AnkiConnect: Connected");
        if (m_AnkiCardSettingsSection) {
          m_AnkiCardSettingsSection->RefreshData();
        }
      } else {
        m_AnkiConnected.store(false);
        if (m_StatusSection)
          m_StatusSection->SetStatus("AnkiConnect: Not connected (click Connect to retry)");
      }
    }).detach();

    return true;
  }

  void Application::Shutdown()
  {
    CancelAsyncTasks();

    m_VideoSection.reset();
    m_ConfigurationSection.reset();
    m_AnkiCardSettingsSection.reset();
    m_StatusSection.reset();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (m_Renderer) {
      SDL_DestroyRenderer(m_Renderer);
      m_Renderer = nullptr;
    }

    if (m_Window) {
      SDL_DestroyWindow(m_Window);
      m_Window = nullptr;
    }

    SDL_Quit();
  }

  void Application::HandleEvents()
  {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT)
        m_IsRunning = false;
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window))
        m_IsRunning = false;

      // Handle file drop
      if (event.type == SDL_EVENT_DROP_FILE) {
          if (m_VideoSection && event.drop.data) {
              m_VideoSection->LoadVideoFromFile(event.drop.data);
          }
      }
    }
  }

  void Application::Update()
  {
    UpdateAsyncTasks();

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (m_VideoSection)
      m_VideoSection->Update();
  }

  void Application::Render()
  {
    RenderUI();
    RenderExtractModal();

    ImGui::Render();
    SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_Renderer);

    ImGuiIO& io = ImGui::GetIO();
    SDL_SetRenderScale(m_Renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_Renderer);
    SDL_RenderPresent(m_Renderer);
  }

  void Application::RenderUI()
  {
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    static bool first_time = true;
    if (first_time) {
      first_time = false;
      ImGui::DockBuilderRemoveNode(dockspace_id);
      ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

      ImGuiID dock_main_id = dockspace_id;
      ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.06f, nullptr, &dock_main_id);
      ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.40f, nullptr, &dock_main_id);

      ImGui::DockBuilderDockWindow("Video Player", dock_main_id);
      ImGui::DockBuilderDockWindow("Card", dock_right_id);
      ImGui::DockBuilderDockWindow("AnkiConnect", dock_right_id);
      ImGui::DockBuilderDockWindow("Text AI", dock_right_id);
      ImGui::DockBuilderDockWindow("Status", dock_bottom_id);

      ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_main_id);
      if (node)
        node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton;

      node = ImGui::DockBuilderGetNode(dock_bottom_id);
      if (node)
        node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton;

      node = ImGui::DockBuilderGetNode(dock_right_id);
      if (node)
        node->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;

      ImGui::DockBuilderFinish(dockspace_id);
    }

    if (m_VideoSection)
      m_VideoSection->Render();

    if (m_AnkiCardSettingsSection) {
      ImGui::Begin("Card", nullptr, ImGuiWindowFlags_NoCollapse);
      m_AnkiCardSettingsSection->Render();
      ImGui::End();
    }

    if (m_ConfigurationSection) {
      ImGui::Begin("AnkiConnect", nullptr, ImGuiWindowFlags_NoCollapse);
      m_ConfigurationSection->RenderAnkiConnectTab();
      ImGui::End();

      ImGui::Begin("Text AI", nullptr, ImGuiWindowFlags_NoCollapse);
      m_ConfigurationSection->RenderTextAITab();
      ImGui::End();
    }

    if (m_StatusSection)
      m_StatusSection->Render();
  }

  void Application::OnExtract()
  {
    if (m_IsExtracting.load()) {
      AF_WARN("Extraction already in progress, ignoring request.");
      return;
    }

    AF_INFO("Starting Extraction...");
    if (m_StatusSection)
      m_StatusSection->SetStatus("Extracting data from video...");

    if (!m_AnkiConnected.load()) {
      if (m_StatusSection)
        m_StatusSection->SetStatus("Error: Anki is not connected.");
      AF_ERROR("Anki is not connected.");
      return;
    }

    // 1. Get current timestamp (implicitly used for audio/image)
    // 2. Take current frame as image
    m_ExtractedImage = m_VideoSection->GetCurrentFrameImage();
    if (m_ExtractedImage.empty()) {
        AF_ERROR("Failed to extract image from video.");
        if (m_StatusSection) m_StatusSection->SetStatus("Error: Failed to extract image.");
        return;
    }

    // 3. Extract sentence from current sub
    auto subtitle = m_VideoSection->GetCurrentSubtitle();
    m_ExtractSentence = subtitle.text;
    for (auto& c : m_ExtractSentence) {
        if (c == '\n' || c == '\r') c = ' ';
    }

    // 4. Extract audio from specific sub
    if (subtitle.end > subtitle.start) {
        m_ExtractedAudio = m_VideoSection->GetAudioClip(subtitle.start, subtitle.end);
    } else {
        // Fallback if no subtitle timing, grab 5 seconds around current time?
        // Or just don't grab audio.
        double current = m_VideoSection->GetCurrentTimestamp();
        m_ExtractedAudio = m_VideoSection->GetAudioClip(current, current + 5.0);
    }

    // 5. Show the modal
    m_ExtractTargetWord = "";
    m_ShowExtractModal = true;
    m_OpenExtractModal = true;

    // Auto-fill Image field immediately
    if (m_AnkiCardSettingsSection) {
        m_AnkiCardSettingsSection->SetFieldByTool(7, m_ExtractedImage, "image.webp");
    }

    if (m_StatusSection)
        m_StatusSection->SetStatus("Extraction complete. Please verify data.");
  }

  void Application::RenderExtractModal()
  {
    if (m_OpenExtractModal) {
      ImGui::OpenPopup("Extract Result");
      m_OpenExtractModal = false;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 0), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginPopupModal("Extract Result", &m_ShowExtractModal, ImGuiWindowFlags_AlwaysAutoResize)) {
      auto InputText = [](const char* label, std::string* str) {
        auto callback = [](ImGuiInputTextCallbackData* data) {
          if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            std::string* s = (std::string*) data->UserData;
            IM_ASSERT(data->Buf == s->c_str());
            s->resize(data->BufTextLen);
            data->Buf = (char*) s->data();
          }
          return 0;
        };
        return ImGui::InputText(
            label, (char*) str->data(), str->capacity() + 1, ImGuiInputTextFlags_CallbackResize, callback, (void*) str);
      };

      auto InputTextMultiline = [](const char* label, std::string* str) {
        auto callback = [](ImGuiInputTextCallbackData* data) {
          if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            std::string* s = (std::string*) data->UserData;
            IM_ASSERT(data->Buf == s->c_str());
            s->resize(data->BufTextLen);
            data->Buf = (char*) s->data();
          }
          return 0;
        };
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        return ImGui::InputTextMultiline(
            label, (char*) str->data(), str->capacity() + 1, ImVec2(-1, 120), ImGuiInputTextFlags_CallbackResize, callback, (void*) str);
      };

      InputTextMultiline("Sentence", &m_ExtractSentence);
      InputText("Target Word", &m_ExtractTargetWord);

      ImGui::Separator();

      // Disable Process button if already processing
      bool isProcessing = m_IsProcessing.load();
      if (isProcessing) {
        ImGui::BeginDisabled();
      }

      // Green tint for Process button
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.60f, 0.20f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.75f, 0.25f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.50f, 0.15f, 1.0f));

      if (ImGui::Button("Process", ImVec2(120, 0))) {
        ProcessExtract();
        m_ShowExtractModal = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::PopStyleColor(3);

      if (isProcessing) {
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Processing...");
      } else {
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        // Red tint for Cancel button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.15f, 0.15f, 1.0f));

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          m_ShowExtractModal = false;
          ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleColor(3);
      }

      ImGui::EndPopup();
    }
  }

  void Application::ProcessExtract()
  {
    // Prevent multiple simultaneous processing
    if (m_IsProcessing.load()) {
      AF_WARN("Processing already in progress, ignoring request.");
      return;
    }

    AF_INFO("Processing Extract. Sentence: '{}', Target Word: '{}'", m_ExtractSentence, m_ExtractTargetWord);
    if (m_StatusSection)
      m_StatusSection->SetStatus("Processing extraction...");

    // Capture needed data for async task
    std::string sentence = m_ExtractSentence;
    std::string targetWord = m_ExtractTargetWord;
    std::vector<unsigned char> audioData = m_ExtractedAudio;
    std::vector<unsigned char> imageData = m_ExtractedImage;

    m_IsProcessing.store(true);

    // Launch processing task in background thread
    AF_INFO("Launching async processing task...");
    if (m_StatusSection)
      m_StatusSection->SetProgress(0.1f);

    {
      std::lock_guard<std::mutex> lock(m_ResultMutex);
      m_LastError.clear();
    }

    AsyncTask task;
    task.description = "Extract Processing";
    task.future = std::async(std::launch::async, [this, sentence, targetWord, audioData, imageData]() {
      try {
        if (m_CancelRequested.load()) {
          AF_INFO("Processing task cancelled before starting.");
          return;
        }
        AF_INFO("Analyzing sentence...");
        AF_DEBUG("Sentence: '{}', Target Word: '{}'", sentence, targetWord);
        nlohmann::json analysis = m_ActiveTextAIProvider->AnalyzeSentence(sentence, targetWord, m_ActiveLanguage);

        if (m_CancelRequested.load()) {
          AF_INFO("Processing task cancelled after analysis.");
          return;
        }

        AF_DEBUG("Analysis Response: {}", analysis.dump());
        if (analysis.is_null()) {
          AF_ERROR("Analysis returned null/empty response");
          throw std::runtime_error("Text analysis failed.");
        }

        if (m_StatusSection)
          m_StatusSection->SetProgress(0.5f);

        AF_INFO("Analysis Result: {}", analysis.dump());

        std::string analyzedSentence = analysis.value("sentence", "");
        std::string translation = analysis.value("translation", "");
        std::string analyzedTargetWord = analysis.value("target_word", "");
        std::string targetWordFurigana = analysis.value("target_word_furigana", "");
        std::string furigana = analysis.value("furigana", "");
        std::string definition = analysis.value("definition", "");
        std::string pitch = analysis.value("pitch_accent", "");

        auto updateFields = [this,
                             analyzedSentence,
                             translation,
                             analyzedTargetWord,
                             targetWordFurigana,
                             furigana,
                             definition,
                             pitch,
                             imageData,
                             audioData]() {
          if (m_AnkiCardSettingsSection) {
            AF_INFO("Setting fields in Anki Card Settings...");
            m_AnkiCardSettingsSection->SetFieldByTool(0, analyzedSentence);
            m_AnkiCardSettingsSection->SetFieldByTool(1, furigana);
            m_AnkiCardSettingsSection->SetFieldByTool(2, translation);
            m_AnkiCardSettingsSection->SetFieldByTool(3, analyzedTargetWord);
            m_AnkiCardSettingsSection->SetFieldByTool(4, targetWordFurigana);
            m_AnkiCardSettingsSection->SetFieldByTool(5, pitch);
            m_AnkiCardSettingsSection->SetFieldByTool(6, definition);

            if (!imageData.empty()) {
              m_AnkiCardSettingsSection->SetFieldByTool(7, imageData, "image.webp");
            }

            if (!audioData.empty()) {
                // 9: Sentence Audio
                m_AnkiCardSettingsSection->SetFieldByTool(9, audioData, "sentence.mp3");
            }
          } else {
            AF_WARN("AnkiCardSettingsSection is null, cannot set fields.");
          }
        };

        updateFields();

        if (m_StatusSection)
          m_StatusSection->SetProgress(1.0f);
        AF_INFO("Processing complete.");
      } catch (const std::exception& e) {
        AF_ERROR("Processing task failed with exception: {}", e.what());
        std::lock_guard<std::mutex> lock(m_ResultMutex);
        m_LastError = std::string("Processing failed: ") + e.what();
      } catch (...) {
        AF_ERROR("Processing task failed with unknown exception.");
        std::lock_guard<std::mutex> lock(m_ResultMutex);
        m_LastError = "Processing failed with unknown error.";
      }
    });

    task.onComplete = [this]() {
      m_IsProcessing.store(false);

      std::string error;
      {
        std::lock_guard<std::mutex> lock(m_ResultMutex);
        error = m_LastError;
      }

      if (!error.empty()) {
        if (m_StatusSection)
          m_StatusSection->SetStatus("Error: " + error);
        AF_ERROR("Processing failed: {}", error);
        return;
      }

      if (m_StatusSection)
        m_StatusSection->SetStatus("Processing complete.");
      AF_INFO("All processing tasks completed successfully.");
    };

    task.onError = [this](const std::string& error) {
      m_IsProcessing.store(false);
      if (m_StatusSection) {
        m_StatusSection->SetStatus("Error: " + error);
        m_StatusSection->SetProgress(-1.0f);
      }
      AF_ERROR("Processing error: {}", error);
    };

    // Add task to queue
    {
      std::lock_guard<std::mutex> lock(m_TaskMutex);
      m_ActiveTasks.push(std::move(task));
    }
  }

  void Application::UpdateAsyncTasks()
  {
    std::lock_guard<std::mutex> lock(m_TaskMutex);
    if (!m_ActiveTasks.empty()) {
      auto& task = m_ActiveTasks.front();
      if (task.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        try {
          task.future.get();
          if (task.onComplete) {
            task.onComplete();
          }
        } catch (const std::exception& e) {
          if (task.onError) {
            task.onError(e.what());
          }
        }
        m_ActiveTasks.pop();
      }
    }
  }

  void Application::CancelAsyncTasks()
  {
    m_CancelRequested.store(true);
    // Wait for current task to finish or check cancel flag
    std::lock_guard<std::mutex> lock(m_TaskMutex);
    while (!m_ActiveTasks.empty()) {
      m_ActiveTasks.pop();
    }
    m_CancelRequested.store(false);
  }

} // namespace Video2Card

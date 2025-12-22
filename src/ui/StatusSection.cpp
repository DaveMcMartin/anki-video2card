#include "ui/StatusSection.h"

#include <imgui.h>

namespace Video2Card::UI
{

  StatusSection::StatusSection()
      : m_StatusMessage("Ready")
  {}

  StatusSection::~StatusSection() {}

  void StatusSection::Render()
  {
    ImGui::Begin("Status");
    if (m_Progress >= 0.0f) {
      ImGui::ProgressBar(m_Progress, ImVec2(200.0f, 0.0f));
      ImGui::SameLine();
    }
    ImGui::Text("%s", m_StatusMessage.c_str());
    ImGui::End();
  }

  void StatusSection::SetStatus(const std::string& status)
  {
    m_StatusMessage = status;
  }

  void StatusSection::SetProgress(float progress)
  {
    m_Progress = progress;
  }

} // namespace Video2Card::UI

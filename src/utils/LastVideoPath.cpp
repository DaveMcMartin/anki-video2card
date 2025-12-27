#include "LastVideoPath.h"

#include <filesystem>
#include <fstream>

#include "FileUtils.h"
#include "core/Logger.h"

namespace Video2Card::Utils
{
  constexpr const char* LAST_VIDEO_PATH_FILENAME = "last_video_path.txt";

  std::string LastVideoPath::GetLastVideoPathFile()
  {
    std::string cacheDir = FileUtils::GetCachePath();
    if (cacheDir.empty()) {
      AF_ERROR("LastVideoPath: Failed to get cache directory");
      return "";
    }
    return cacheDir + LAST_VIDEO_PATH_FILENAME;
  }

  std::optional<std::string> LastVideoPath::Load()
  {
    std::string pathFile = GetLastVideoPathFile();
    if (pathFile.empty()) {
      AF_DEBUG("LastVideoPath: Could not determine state file path");
      return std::nullopt;
    }

    std::ifstream file(pathFile);
    if (!file.is_open()) {
      AF_DEBUG("LastVideoPath: No saved video path found at: {}", pathFile);
      return std::nullopt;
    }

    std::string videoPath;
    std::getline(file, videoPath);
    file.close();

    if (videoPath.empty()) {
      AF_DEBUG("LastVideoPath: Saved path is empty");
      return std::nullopt;
    }

    // Validate that the file still exists
    if (!std::filesystem::exists(videoPath)) {
      AF_WARN("LastVideoPath: Saved video file no longer exists: {}", videoPath);
      return std::nullopt;
    }

    AF_INFO("LastVideoPath: Loaded video path: {}", videoPath);
    return videoPath;
  }

  bool LastVideoPath::Save(const std::string& filePath)
  {
    if (filePath.empty()) {
      AF_WARN("LastVideoPath: Cannot save empty file path");
      return false;
    }

    std::string pathFile = GetLastVideoPathFile();
    if (pathFile.empty()) {
      AF_WARN("LastVideoPath: Could not determine state file path for save");
      return false;
    }

    std::ofstream file(pathFile);
    if (!file.is_open()) {
      AF_WARN("LastVideoPath: Failed to open file for writing: {}", pathFile);
      return false;
    }

    file << filePath;
    file.close();

    AF_DEBUG("LastVideoPath: Saved video path: {}", filePath);
    return true;
  }

  bool LastVideoPath::Clear()
  {
    std::string pathFile = GetLastVideoPathFile();
    if (pathFile.empty()) {
      AF_WARN("LastVideoPath: Could not determine state file path for clear");
      return false;
    }

    try {
      if (std::filesystem::remove(pathFile)) {
        AF_INFO("LastVideoPath: Cleared saved video path file: {}", pathFile);
        return true;
      }
      AF_DEBUG("LastVideoPath: No saved video path file to remove: {}", pathFile);
      return false;
    } catch (const std::exception& e) {
      AF_WARN("LastVideoPath: Error clearing saved video path: {}", e.what());
      return false;
    }
  }
} // namespace Video2Card::Utils
#include "VideoState.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "FileUtils.h"
#include "core/Logger.h"

namespace Video2Card::Utils
{
  constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
  constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;
  constexpr size_t CHUNK_SIZE = 4096;

  static uint64_t Fnv1aHash(const uint8_t* data, size_t length)
  {
    uint64_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < length; ++i) {
      hash ^= data[i];
      hash *= FNV_PRIME;
    }
    return hash;
  }

  uint64_t VideoState::ComputeFileHash(const std::string& filePath)
  {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
      AF_WARN("VideoState: Failed to open file for hashing: {}", filePath);
      return 0;
    }

    file.seekg(0, std::ios::end);
    uint64_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize == 0) {
      AF_WARN("VideoState: File is empty: {}", filePath);
      return 0;
    }

    uint64_t hash = FNV_OFFSET_BASIS;

    uint8_t sizeBytes[8];
    std::memcpy(sizeBytes, &fileSize, sizeof(uint64_t));
    hash = Fnv1aHash(sizeBytes, 8);

    uint8_t buffer[CHUNK_SIZE];
    file.read(reinterpret_cast<char*>(buffer), CHUNK_SIZE);
    size_t bytesRead = file.gcount();
    hash = Fnv1aHash(buffer, bytesRead);

    if (fileSize > CHUNK_SIZE) {
      file.seekg(-static_cast<int64_t>(CHUNK_SIZE), std::ios::end);
      file.read(reinterpret_cast<char*>(buffer), CHUNK_SIZE);
      bytesRead = file.gcount();
      hash = Fnv1aHash(buffer, bytesRead);
    }

    file.close();
    AF_DEBUG("VideoState: Computed hash for {}: {:#x}", filePath, hash);
    return hash;
  }

  std::string VideoState::GetStateFilePath(uint64_t fileHash)
  {
    std::string cacheDir = FileUtils::GetCachePath();
    if (cacheDir.empty()) {
      AF_ERROR("VideoState: Failed to get cache directory");
      return "";
    }

    std::stringstream ss;
    ss << std::hex << fileHash;
    std::string stateFile = cacheDir + "video_" + ss.str() + ".state";
    return stateFile;
  }

  uint64_t VideoState::LoadPlaybackPosition(const std::string& filePath)
  {
    uint64_t fileHash = ComputeFileHash(filePath);
    if (fileHash == 0) {
      AF_DEBUG("VideoState: Could not compute hash for: {}", filePath);
      return 0;
    }

    std::string stateFile = GetStateFilePath(fileHash);
    if (stateFile.empty()) {
      AF_DEBUG("VideoState: Could not determine state file path");
      return 0;
    }

    std::ifstream file(stateFile);
    if (!file.is_open()) {
      AF_DEBUG("VideoState: No saved state found at: {}", stateFile);
      return 0;
    }

    uint64_t position = 0;
    file >> position;
    file.close();

    if (position > 0) {
      AF_INFO("VideoState: Loaded position {} ms from: {}", position, stateFile);
    }

    return position;
  }

  bool VideoState::SavePlaybackPosition(const std::string& filePath, uint64_t positionMs)
  {
    uint64_t fileHash = ComputeFileHash(filePath);
    if (fileHash == 0) {
      AF_WARN("VideoState: Could not compute hash for save: {}", filePath);
      return false;
    }

    std::string stateFile = GetStateFilePath(fileHash);
    if (stateFile.empty()) {
      AF_WARN("VideoState: Could not determine state file path for save");
      return false;
    }

    std::ofstream file(stateFile);
    if (!file.is_open()) {
      AF_WARN("VideoState: Failed to open state file for writing: {}", stateFile);
      return false;
    }

    file << positionMs;
    file.close();

    AF_DEBUG("VideoState: Saved position {} ms to: {}", positionMs, stateFile);
    return true;
  }

  bool VideoState::ClearPlaybackPosition(const std::string& filePath)
  {
    uint64_t fileHash = ComputeFileHash(filePath);
    if (fileHash == 0) {
      AF_WARN("VideoState: Could not compute hash for clear: {}", filePath);
      return false;
    }

    std::string stateFile = GetStateFilePath(fileHash);
    if (stateFile.empty()) {
      AF_WARN("VideoState: Could not determine state file path for clear");
      return false;
    }

    try {
      if (std::filesystem::remove(stateFile)) {
        AF_INFO("VideoState: Cleared state file: {}", stateFile);
        return true;
      }
      AF_DEBUG("VideoState: State file did not exist: {}", stateFile);
      return false;
    } catch (const std::exception& e) {
      AF_WARN("VideoState: Error clearing state file: {}", e.what());
      return false;
    }
  }
} // namespace Video2Card::Utils
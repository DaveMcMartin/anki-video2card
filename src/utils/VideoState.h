#pragma once

#include <string>
#include <cstdint>

namespace Video2Card::Utils
{
  class VideoState
  {
  public:
    static uint64_t LoadPlaybackPosition(const std::string& filePath);
    static bool SavePlaybackPosition(const std::string& filePath, uint64_t positionMs);
    static bool ClearPlaybackPosition(const std::string& filePath);

  private:
    static uint64_t ComputeFileHash(const std::string& filePath);
    static std::string GetStateFilePath(uint64_t fileHash);
  };
}
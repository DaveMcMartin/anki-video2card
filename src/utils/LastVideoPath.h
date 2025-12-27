#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace Video2Card::Utils
{
  /**
   * Utility class for managing the last loaded video path.
   * Persists the video file path so it can be restored when the application restarts.
   * 
   * This follows the Single Responsibility Principle: it only manages the last video path.
   * State is stored in a simple text file in the cache directory.
   */
  class LastVideoPath
  {
public:

    /**
     * Loads the last saved video path from persistent storage.
     * 
     * @return The last video path if it exists and is valid, std::nullopt otherwise
     */
    static std::optional<std::string> Load();

    /**
     * Saves the current video path to persistent storage.
     * 
     * @param filePath The path to save
     * @return true if save was successful, false otherwise
     */
    static bool Save(const std::string& filePath);

    /**
     * Clears the saved video path from persistent storage.
     * 
     * @return true if clear was successful, false otherwise
     */
    static bool Clear();

private:

    static std::string GetLastVideoPathFile();
  };
} // namespace Video2Card::Utils
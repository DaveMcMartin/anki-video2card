#pragma once

#include <string>

namespace Video2Card::Utils
{
  class FileUtils
  {
public:

    /**
     * Gets the platform-specific directory for storing application preferences/config files.
     * Uses SDL_GetPrefPath internally.
     * - macOS: ~/Library/Application Support/AnkiVideo2Card/
     * - Windows: %APPDATA%/AnkiVideo2Card/
     * - Linux: ~/.local/share/AnkiVideo2Card/
     * 
     * @return The preference path as a string with trailing separator
     */
    static std::string GetPrefPath();

    /**
     * Gets the full path to the config file.
     * 
     * @return Full path to config.json in the preference directory
     */
    static std::string GetConfigPath();

    /**
     * Gets the platform-specific directory for storing cache/temporary files.
     * Uses SDL_GetPrefPath internally with a "cache" subdirectory.
     * - macOS: ~/Library/Caches/AnkiVideo2Card/
     * - Windows: %APPDATA%/AnkiVideo2Card/Cache/
     * - Linux: ~/.cache/AnkiVideo2Card/
     * 
     * @return The cache path as a string with trailing separator
     */
    static std::string GetCachePath();
  };
} // namespace Video2Card::Utils
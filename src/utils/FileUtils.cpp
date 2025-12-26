#include "FileUtils.h"
#include <SDL3/SDL.h>
#include <sys/stat.h>
#include <cstdlib>

#ifdef _WIN32
#include <direct.h>
#endif

namespace Video2Card::Utils
{
  std::string FileUtils::GetPrefPath()
  {
    char* prefPath = SDL_GetPrefPath("com.ankivideo2card", "AnkiVideo2Card");
    if (!prefPath) {
      return "";
    }

    std::string result(prefPath);
    SDL_free(prefPath);
    return result;
  }

  std::string FileUtils::GetConfigPath()
  {
    return GetPrefPath() + "config.json";
  }

  std::string FileUtils::GetCachePath()
  {
#ifdef __APPLE__
    // macOS: ~/Library/Caches/AnkiVideo2Card/
    const char* home = std::getenv("HOME");
    if (home) {
      std::string cachePath = std::string(home) + "/Library/Caches/AnkiVideo2Card/";
      // Try to create directory (may fail silently if it exists)
      mkdir(cachePath.c_str(), 0755);
      return cachePath;
    }
#elif defined(_WIN32)
    // Windows: Use APPDATA/AnkiVideo2Card/Cache/
    char* prefPath = SDL_GetPrefPath("com.ankivideo2card", "AnkiVideo2Card");
    if (prefPath) {
      std::string cachePath = std::string(prefPath) + "Cache/";
      _mkdir(cachePath.c_str());
      SDL_free(prefPath);
      return cachePath;
    }
#else
    // Linux: ~/.cache/AnkiVideo2Card/
    const char* home = std::getenv("HOME");
    if (home) {
      std::string cachePath = std::string(home) + "/.cache/AnkiVideo2Card/";
      mkdir(cachePath.c_str(), 0755);
      return cachePath;
    }
#endif
    return "";
  }
} // namespace Video2Card::Utils
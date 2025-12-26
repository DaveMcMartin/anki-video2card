#include "FileUtils.h"
#include <SDL3/SDL.h>

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
} // namespace Video2Card::Utils
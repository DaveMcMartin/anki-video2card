#pragma once

#include <SDL3/SDL.h>

#include <memory>

namespace Video2Card::SDL
{

  struct SurfaceDeleter
  {
    void operator()(SDL_Surface* surface) const
    {
      if (surface) {
        SDL_DestroySurface(surface);
      }
    }
  };

  struct NonOwningDeleter
  {
    void operator()(SDL_Surface* surface) const
    {
      if (surface) {
        SDL_free(surface);
      }
    }
  };

  using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;
  using NonOwningSurfacePtr = std::unique_ptr<SDL_Surface, NonOwningDeleter>;

  inline SurfacePtr MakeSurface(int width, int height, SDL_PixelFormat format)
  {
    return SurfacePtr(SDL_CreateSurface(width, height, format));
  }

  inline NonOwningSurfacePtr MakeSurfaceFrom(int width, int height, SDL_PixelFormat format, void* pixels, int pitch)
  {
    return NonOwningSurfacePtr(SDL_CreateSurfaceFrom(width, height, format, pixels, pitch));
  }

} // namespace Video2Card::SDL

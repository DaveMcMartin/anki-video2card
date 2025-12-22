#include "utils/ImageProcessor.h"

#include <algorithm>
#include <cstring>

#include <SDL3/SDL.h>
#include <webp/encode.h>

#include "stb_image.h"
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include "core/Logger.h"
#include "core/sdl/SDLWrappers.h"

namespace Video2Card::Utils
{

  SDL_Surface* ImageProcessor::LoadImageFromBuffer(const std::vector<unsigned char>& imageBuffer)
  {
    if (imageBuffer.empty()) {
      AF_ERROR("Image buffer is empty");
      return nullptr;
    }

    int width{}, height{}, channels{};
    unsigned char* data = stbi_load_from_memory(
        imageBuffer.data(), imageBuffer.size(), &width, &height, &channels, 4);

    if (!data) {
      AF_ERROR("Failed to load image from buffer: {}", stbi_failure_reason());
      return nullptr;
    }

    // Create a duplicate surface that we own, since MakeSurfaceFrom doesn't own the pixel data
    auto tempSurface = SDL::MakeSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, width * 4);
    if (!tempSurface) {
      AF_ERROR("Failed to create SDL surface: {}", SDL_GetError());
      stbi_image_free(data);
      return nullptr;
    }

    // Duplicate the surface so we own the pixel data
    SDL_Surface* surface = SDL_DuplicateSurface(tempSurface.get());
    stbi_image_free(data);

    if (!surface) {
      AF_ERROR("Failed to duplicate surface: {}", SDL_GetError());
      return nullptr;
    }

    return surface;
  }

  void ImageProcessor::CalculateScaledDimensions(int srcWidth, int srcHeight, int maxWidth, int maxHeight,
                                                 int& outWidth, int& outHeight)
  {
    float srcAspect = static_cast<float>(srcWidth) / static_cast<float>(srcHeight);
    float maxAspect = static_cast<float>(maxWidth) / static_cast<float>(maxHeight);

    if (srcAspect > maxAspect) {
      // Source is wider - constrain by width
      outWidth = maxWidth;
      outHeight = static_cast<int>(maxWidth / srcAspect);
    } else {
      // Source is taller - constrain by height
      outHeight = maxHeight;
      outWidth = static_cast<int>(maxHeight * srcAspect);
    }

    // Ensure dimensions are at least 1
    outWidth = std::max(1, outWidth);
    outHeight = std::max(1, outHeight);
  }

  std::vector<unsigned char> ImageProcessor::SurfaceToWebP(SDL_Surface* surface, int qualityPercent)
  {
    if (!surface) {
      AF_ERROR("Surface is null");
      return {};
    }

    std::vector<unsigned char> result;

    // Use WebP encoder - lossless RGBA encoding
    uint8_t* output = nullptr;
    size_t output_size = WebPEncodeLosslessRGBA(
        static_cast<const uint8_t*>(surface->pixels), surface->w, surface->h, surface->pitch, &output);

    if (output_size == 0) {
      AF_ERROR("WebP encoding failed");
      if (output)
        free(output);
      return {};
    }

    result.assign(output, output + output_size);
    free(output);

    AF_INFO("Encoded WebP: {}x{}, size: {} bytes, quality: {}", surface->w, surface->h, output_size,
            qualityPercent);
    return result;
  }

  std::vector<unsigned char> ImageProcessor::ScaleAndCompressToWebP(const std::vector<unsigned char>& imageBuffer,
                                                                    int maxWidth,
                                                                    int maxHeight,
                                                                    int qualityPercent)
  {
    if (imageBuffer.empty()) {
      AF_ERROR("Image buffer is empty");
      return {};
    }

    SDL_Surface* surface = LoadImageFromBuffer(imageBuffer);
    if (!surface) {
      return {};
    }

    // Check if scaling is needed
    if (surface->w <= maxWidth && surface->h <= maxHeight) {
      // No scaling needed, just compress
      auto result = SurfaceToWebP(surface, qualityPercent);
      SDL_DestroySurface(surface);
      return result;
    }

    // Calculate new dimensions maintaining aspect ratio
    int newWidth{}, newHeight{};
    CalculateScaledDimensions(surface->w, surface->h, maxWidth, maxHeight, newWidth, newHeight);

    // Create scaled surface
    auto scaledSurface = SDL::MakeSurface(newWidth, newHeight, surface->format);
    if (!scaledSurface) {
      AF_ERROR("Failed to create scaled surface");
      SDL_DestroySurface(surface);
      return {};
    }

    // Resize using stbir
    const unsigned char* srcPixels = static_cast<const unsigned char*>(surface->pixels);
    unsigned char* dstPixels = static_cast<unsigned char*>(scaledSurface->pixels);

    stbir_resize_uint8_srgb(srcPixels, surface->w, surface->h, surface->pitch, dstPixels, newWidth, newHeight,
                            scaledSurface->pitch, STBIR_RGBA); // RGBA pixel layout

    // Encode to WebP
    auto result = SurfaceToWebP(scaledSurface.get(), qualityPercent);

    AF_INFO("Scaled and compressed to WebP: {}x{} -> {}x{}", surface->w, surface->h, newWidth, newHeight);

    SDL_DestroySurface(surface);
    return result;
  }

  std::vector<unsigned char> ImageProcessor::CompressToWebP(const std::vector<unsigned char>& imageBuffer,
                                                            int qualityPercent)
  {
    if (imageBuffer.empty()) {
      AF_ERROR("Image buffer is empty");
      return {};
    }

    SDL_Surface* surface = LoadImageFromBuffer(imageBuffer);
    if (!surface) {
      return {};
    }

    auto result = SurfaceToWebP(surface, qualityPercent);
    SDL_DestroySurface(surface);
    return result;
  }

} // namespace Video2Card::Utils

#pragma once

#include <memory>
#include <string>
#include <vector>

struct SDL_Surface;

namespace Video2Card::Utils
{

  class ImageProcessor
  {
public:

    /**
     * Scale and compress image to fit in a maximum size while maintaining aspect ratio
     * Used for adding images to Anki cards
     * @param imageBuffer Input image buffer
     * @param maxWidth Maximum width
     * @param maxHeight Maximum height
     * @param qualityPercent Quality for WebP compression (1-100)
     * @return Compressed image buffer in WebP format
     */
    static std::vector<unsigned char> ScaleAndCompressToWebP(const std::vector<unsigned char>& imageBuffer,
                                                             int maxWidth,
                                                             int maxHeight,
                                                             int qualityPercent = 75);

    /**
     * Compress an image to WebP format
     * @param imageBuffer Input image buffer
     * @param qualityPercent Quality for WebP compression (1-100)
     * @return Compressed image buffer in WebP format
     */
    static std::vector<unsigned char> CompressToWebP(const std::vector<unsigned char>& imageBuffer,
                                                     int qualityPercent = 75);

private:

    /**
     * Load image from buffer into SDL_Surface
     * Returns raw pointer - caller must call SDL_DestroySurface when done
     */
    static SDL_Surface* LoadImageFromBuffer(const std::vector<unsigned char>& imageBuffer);

    /**
     * Convert SDL_Surface to WebP format
     */
    static std::vector<unsigned char> SurfaceToWebP(SDL_Surface* surface, int qualityPercent);

    /**
     * Calculate scaled dimensions maintaining aspect ratio
     */
    static void CalculateScaledDimensions(int srcWidth, int srcHeight, int maxWidth, int maxHeight,
                                          int& outWidth, int& outHeight);
  };

} // namespace Video2Card::Utils

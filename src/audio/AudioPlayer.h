#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Video2Card::Audio
{
  class AudioPlayer
  {
public:

    AudioPlayer();
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    bool Play(const std::vector<unsigned char>& data);

    void Stop();

private:

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
  };
} // namespace Video2Card::Audio

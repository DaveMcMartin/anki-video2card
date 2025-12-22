#define MINIAUDIO_IMPLEMENTATION
#include "AudioPlayer.h"

#include "../../third_party/miniaudio.h"
#include "core/Logger.h"

namespace Video2Card::Audio
{
  struct AudioPlayer::Impl
  {
    ma_device device;
    ma_decoder decoder;
    bool isDeviceInitialized = false;
    bool isDecoderInitialized = false;
    std::vector<unsigned char> audioBuffer;

    static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
    {
      auto* pDecoder = static_cast<ma_decoder*>(pDevice->pUserData);
      if (!pDecoder)
        return;

      ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, nullptr);
    }

    void Reset()
    {
      if (isDeviceInitialized) {
        ma_device_uninit(&device);
        isDeviceInitialized = false;
      }

      if (isDecoderInitialized) {
        ma_decoder_uninit(&decoder);
        isDecoderInitialized = false;
      }

      audioBuffer.clear();
    }
  };

  AudioPlayer::AudioPlayer()
      : m_Impl(std::make_unique<Impl>())
  {}

  AudioPlayer::~AudioPlayer()
  {
    Stop();
  }

  bool AudioPlayer::Play(const std::vector<unsigned char>& data)
  {
    AF_INFO("AudioPlayer::Play called with {} bytes", data.size());
    Stop();

    if (data.empty()) {
      return false;
    }

    m_Impl->audioBuffer = data;

    ma_decoder_config decoderConfig = ma_decoder_config_init_default();
    ma_result result = ma_decoder_init_memory(
        m_Impl->audioBuffer.data(), m_Impl->audioBuffer.size(), &decoderConfig, &m_Impl->decoder);

    if (result != MA_SUCCESS) {
      AF_ERROR("Failed to initialize audio decoder");
      return false;
    }
    m_Impl->isDecoderInitialized = true;

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = m_Impl->decoder.outputFormat;
    deviceConfig.playback.channels = m_Impl->decoder.outputChannels;
    deviceConfig.sampleRate = m_Impl->decoder.outputSampleRate;
    deviceConfig.dataCallback = Impl::DataCallback;
    deviceConfig.pUserData = &m_Impl->decoder;

    result = ma_device_init(nullptr, &deviceConfig, &m_Impl->device);
    if (result != MA_SUCCESS) {
      AF_ERROR("Failed to initialize playback device");
      Stop();
      return false;
    }
    m_Impl->isDeviceInitialized = true;

    result = ma_device_start(&m_Impl->device);
    if (result != MA_SUCCESS) {
      AF_ERROR("Failed to start playback device");
      Stop();
      return false;
    }

    return true;
  }

  void AudioPlayer::Stop()
  {
    m_Impl->Reset();
  }
} // namespace Video2Card::Audio

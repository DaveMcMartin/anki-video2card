#define MINIAUDIO_IMPLEMENTATION
#include "AudioPlayer.h"

#include "../../third_party/miniaudio.h"
#include "core/Logger.h"

#define STB_VORBIS_NO_PUSHDATA_API
#include "../../third_party/stb_vorbis.c"

namespace Video2Card::Audio
{
  struct AudioPlayer::Impl
  {
    ma_device device;
    ma_decoder decoder;
    bool isDeviceInitialized = false;
    bool isDecoderInitialized = false;
    bool isRawPCM = false;
    std::vector<unsigned char> audioBuffer;
    std::vector<short> pcmBuffer;
    size_t pcmPlaybackPosition = 0;
    ma_uint32 sampleRate = 0;
    ma_uint32 channels = 0;

    static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
    {
      (void) pInput;
      auto* pImpl = static_cast<Impl*>(pDevice->pUserData);
      if (!pImpl)
        return;

      if (pImpl->isRawPCM) {
        size_t samplesToRead = frameCount * pImpl->channels;
        size_t samplesAvailable = pImpl->pcmBuffer.size() - pImpl->pcmPlaybackPosition;
        size_t samplesToCopy = std::min(samplesToRead, samplesAvailable);

        if (samplesToCopy > 0) {
          std::memcpy(pOutput, &pImpl->pcmBuffer[pImpl->pcmPlaybackPosition], samplesToCopy * sizeof(short));
          pImpl->pcmPlaybackPosition += samplesToCopy;
        }

        if (samplesToCopy < samplesToRead) {
          std::memset(
              (char*) pOutput + (samplesToCopy * sizeof(short)), 0, (samplesToRead - samplesToCopy) * sizeof(short));
        }
      } else {
        ma_decoder_read_pcm_frames(&pImpl->decoder, pOutput, frameCount, nullptr);
      }
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
      pcmBuffer.clear();
      pcmPlaybackPosition = 0;
      isRawPCM = false;
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

    ma_device_config deviceConfig;

    if (result != MA_SUCCESS) {
      AF_WARN("miniaudio decoder failed (result: {}), trying stb_vorbis for OGG support", static_cast<int>(result));

      int error = 0;
      stb_vorbis* vorbis = stb_vorbis_open_memory(
          m_Impl->audioBuffer.data(), static_cast<int>(m_Impl->audioBuffer.size()), &error, nullptr);

      if (!vorbis) {
        AF_ERROR("Failed to initialize audio decoder with stb_vorbis (error: {})", error);
        return false;
      }

      stb_vorbis_info info = stb_vorbis_get_info(vorbis);
      AF_INFO("OGG file: {} channels, {} Hz", info.channels, info.sample_rate);

      unsigned int totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
      m_Impl->pcmBuffer.resize(totalSamples * info.channels);

      int samplesDecoded = stb_vorbis_get_samples_short_interleaved(
          vorbis, info.channels, m_Impl->pcmBuffer.data(), m_Impl->pcmBuffer.size());

      stb_vorbis_close(vorbis);

      if (samplesDecoded <= 0) {
        AF_ERROR("Failed to decode OGG audio data");
        return false;
      }

      m_Impl->pcmBuffer.resize(samplesDecoded * info.channels);
      m_Impl->pcmPlaybackPosition = 0;
      m_Impl->sampleRate = info.sample_rate;
      m_Impl->channels = info.channels;
      m_Impl->isRawPCM = true;

      AF_INFO("Decoded {} samples from OGG", samplesDecoded);

      deviceConfig = ma_device_config_init(ma_device_type_playback);
      deviceConfig.playback.format = ma_format_s16;
      deviceConfig.playback.channels = info.channels;
      deviceConfig.sampleRate = info.sample_rate;
      deviceConfig.dataCallback = Impl::DataCallback;
      deviceConfig.pUserData = m_Impl.get();
    } else {
      m_Impl->isDecoderInitialized = true;
      m_Impl->isRawPCM = false;

      deviceConfig = ma_device_config_init(ma_device_type_playback);
      deviceConfig.playback.format = m_Impl->decoder.outputFormat;
      deviceConfig.playback.channels = m_Impl->decoder.outputChannels;
      deviceConfig.sampleRate = m_Impl->decoder.outputSampleRate;
      deviceConfig.dataCallback = Impl::DataCallback;
      deviceConfig.pUserData = m_Impl.get();
    }

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

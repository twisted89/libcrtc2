#include "customaudiofactory.h"
#include "rtcpeerconnection.h"
#include "customaudiodecoder.h"
#include "api/audio_codecs/audio_decoder_factory_template.h"
#include "api/audio_codecs/opus/audio_decoder_multi_channel_opus.h"
#include "api/audio_codecs/opus/audio_decoder_opus.h"

namespace crtc {

    // Modify an audio decoder to not advertise support for anything.
    template <typename T>
    struct NotAdvertised {
      using Config = typename T::Config;
      static absl::optional<Config> SdpToConfig(
          const webrtc::SdpAudioFormat& audio_format) {
        return T::SdpToConfig(audio_format);
      }
      static void AppendSupportedDecoders(std::vector<webrtc::AudioCodecSpec>* specs) {
        // Don't advertise support for anything.
      }
      static std::unique_ptr<webrtc::AudioDecoder> MakeAudioDecoder(
          const Config& config,
          absl::optional<webrtc::AudioCodecPairId> codec_pair_id = absl::nullopt) {
        return T::MakeAudioDecoder(config, codec_pair_id);
      }
    };

	CustomAudioFactory::CustomAudioFactory(RTCPeerConnectionInternal* pc) :
		_pc(pc)
	{
        _audioFactory = webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderOpus, NotAdvertised<webrtc::AudioDecoderMultiChannelOpus>>();
	}

	CustomAudioFactory::~CustomAudioFactory()
	{
	}

	std::vector<webrtc::AudioCodecSpec> CustomAudioFactory::GetSupportedDecoders()
	{
		return _audioFactory->GetSupportedDecoders();
	}

	bool CustomAudioFactory::IsSupportedDecoder(const webrtc::SdpAudioFormat& format)
	{
        return _audioFactory->IsSupportedDecoder(format);
	}

	std::unique_ptr<webrtc::AudioDecoder> CustomAudioFactory::MakeAudioDecoder(const webrtc::SdpAudioFormat& format,
		absl::optional<webrtc::AudioCodecPairId> codec_pair_id)
	{
		if (_pc->BypassAudioDecoder())
			return std::make_unique<CustomAudioDecoder>(_pc, _audioFactory, format, codec_pair_id);

		return _audioFactory->MakeAudioDecoder(format, codec_pair_id);
	}
}

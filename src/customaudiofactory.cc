#include "customaudiofactory.h"
#include "rtcpeerconnection.h"
#include "customaudiodecoder.h"
#include "api/audio_codecs/opus/audio_decoder_multi_channel_opus.h"
#include "api/audio_codecs/opus/audio_decoder_opus.h"
#include "api/audio_codecs/audio_decoder_factory_template.h"

namespace crtc {

	CustomAudioFactory::CustomAudioFactory(RTCPeerConnectionInternal* pc) :
		_pc(pc)
	{
	}

	CustomAudioFactory::~CustomAudioFactory()
	{
	}

	std::vector<webrtc::AudioCodecSpec> CustomAudioFactory::GetSupportedDecoders()
	{
		std::vector<webrtc::AudioCodecSpec> specs;
		webrtc::audio_decoder_factory_template_impl::Helper<webrtc::AudioDecoderOpus>::AppendSupportedDecoders(&specs);
		return specs;
	}

	bool CustomAudioFactory::IsSupportedDecoder(const webrtc::SdpAudioFormat& format)
	{
		return webrtc::audio_decoder_factory_template_impl::Helper<webrtc::AudioDecoderOpus>::IsSupportedDecoder(format);
	}

	std::unique_ptr<webrtc::AudioDecoder> CustomAudioFactory::MakeAudioDecoder(const webrtc::SdpAudioFormat& format,
		absl::optional<webrtc::AudioCodecPairId> codec_pair_id)
	{
		if (_pc->BypassAudioDecoder())
			return std::make_unique<CustomAudioDecoder>(_pc);

		return webrtc::audio_decoder_factory_template_impl::Helper<webrtc::AudioDecoderOpus>::MakeAudioDecoder(format, codec_pair_id, nullptr);
	}
}

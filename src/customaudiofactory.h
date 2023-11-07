#ifndef CRTC_CUSTOMAUDIOFACTORY_H
#define CRTC_CUSTOMAUDIOFACTORY_H

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "rtc_base/ref_counted_object.h"

namespace crtc {
	class RTCPeerConnectionInternal;
	class CustomAudioFactory : public webrtc::AudioDecoderFactory {
	public:
		CustomAudioFactory(RTCPeerConnectionInternal* pc);
		virtual ~CustomAudioFactory();

		std::vector<webrtc::AudioCodecSpec> GetSupportedDecoders() override;

		bool IsSupportedDecoder(const webrtc::SdpAudioFormat& format) override;

		// Create a new decoder instance. The `codec_pair_id` argument is used to link
		// encoders and decoders that talk to the same remote entity: if a
		// AudioEncoderFactory::MakeAudioEncoder() and a
		// AudioDecoderFactory::MakeAudioDecoder() call receive non-null IDs that
		// compare equal, the factory implementations may assume that the encoder and
		// decoder form a pair. (The intended use case for this is to set up
		// communication between the AudioEncoder and AudioDecoder instances, which is
		// needed for some codecs with built-in bandwidth adaptation.)
		//
		// Returns null if the format isn't supported.
		//
		// Note: Implementations need to be robust against combinations other than
		// one encoder, one decoder getting the same ID; such decoders must still
		// work.
		std::unique_ptr<webrtc::AudioDecoder> MakeAudioDecoder(const webrtc::SdpAudioFormat& format,
			absl::optional<webrtc::AudioCodecPairId> codec_pair_id) override;
	private:
		RTCPeerConnectionInternal* _pc;
	};

}

#endif
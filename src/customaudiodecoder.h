#ifndef CRTC_CUSTOMAUDIODECODER_H
#define CRTC_CUSTOMAUDIODECODER_H

#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_decoder.h"
#include "rtc_base/buffer.h"

namespace crtc {
	class RTCPeerConnectionInternal;
	class CustomAudioDecoder : public webrtc::AudioDecoder {
	public:
		CustomAudioDecoder(RTCPeerConnectionInternal* pc, rtc::scoped_refptr<webrtc::AudioDecoderFactory> audioFactory,
                           const webrtc::SdpAudioFormat& format, absl::optional<webrtc::AudioCodecPairId> codec_pair_id);
		virtual ~CustomAudioDecoder();

        std::vector<webrtc::AudioDecoder::ParseResult> ParsePayload(rtc::Buffer&& payload, uint32_t timestamp) override;

        // Resets the decoder state (empty buffers etc.).
        void Reset() override;

        // Returns the last error code from the decoder.
        int ErrorCode() override;

        // Returns the actual sample rate of the decoder's output. This value may not
        // change during the lifetime of the decoder.
        int SampleRateHz() const override;

        // The number of channels in the decoder's output. This value may not change
        // during the lifetime of the decoder.
        size_t Channels() const override;

    protected:
        int DecodeInternal(const uint8_t* encoded, size_t encoded_len, int sample_rate_hz, int16_t* decoded, SpeechType* speech_type) override;
        int DecodeRedundantInternal(const uint8_t* encoded,
                              size_t encoded_len,
                              int sample_rate_hz,
                              int16_t* decoded,
                              SpeechType* speech_type) override;

	private:
		RTCPeerConnectionInternal* _pc;
        std::unique_ptr<webrtc::AudioDecoder> _decoder;
	};

}

#endif
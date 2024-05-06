#include "customaudiodecoder.h"
#include "rtcpeerconnection.h"
#include "modules/audio_coding/codecs/opus/audio_coder_opus_common.h"

namespace crtc {
	CustomAudioDecoder::CustomAudioDecoder(RTCPeerConnectionInternal* pc, rtc::scoped_refptr<webrtc::AudioDecoderFactory> audioFactory,
                                           const webrtc::SdpAudioFormat& format, absl::optional<webrtc::AudioCodecPairId> codec_pair_id) :
		_pc(pc),
        _decoder(audioFactory->MakeAudioDecoder(format, codec_pair_id))
	{

	}

	CustomAudioDecoder::~CustomAudioDecoder()
	{

	}

	void CustomAudioDecoder::Reset()
	{
	}

    std::vector<webrtc::AudioDecoder::ParseResult> CustomAudioDecoder::ParsePayload(rtc::Buffer&& payload, uint32_t timestamp)
    {
        if (_decoder->PacketHasFec(payload.data(), payload.size())) {
            return _decoder->ParsePayload(std::move(payload), timestamp);
        }
        std::vector<ParseResult> results;
        //std::unique_ptr<EncodedAudioFrame> frame(new webrtc::OpusFrame(this, std::move(payload), true));
        //results.emplace_back(timestamp, 0, std::move(frame));
        _pc->onRawAudio(payload.data(), payload.size());
        return results;
    }

	int CustomAudioDecoder::DecodeInternal(const uint8_t* encoded, size_t encoded_len, int sample_rate_hz, int16_t* decoded, SpeechType* speech_type)
	{
		_pc->onRawAudio(encoded, encoded_len);
		*decoded = 0;
		*speech_type = SpeechType::kSpeech;
		return 0;
	}

    int CustomAudioDecoder::DecodeRedundantInternal(const uint8_t* encoded,
                                                  size_t encoded_len,
                                                  int sample_rate_hz,
                                                  int16_t* decoded,
                                                  SpeechType* speech_type) {
        if (!PacketHasFec(encoded, encoded_len)) {
            return DecodeInternal(encoded, encoded_len, sample_rate_hz, decoded, speech_type);
        }
        return 0;
    }

	int CustomAudioDecoder::ErrorCode()
	{
		return _decoder->ErrorCode();
	}

	int CustomAudioDecoder::SampleRateHz() const
	{
		return _decoder->SampleRateHz();
	}

	size_t CustomAudioDecoder::Channels() const
	{
		return _decoder->Channels();
	}

}
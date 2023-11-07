#include "customaudiodecoder.h"
#include "rtcpeerconnection.h"


namespace crtc {
	CustomAudioDecoder::CustomAudioDecoder(RTCPeerConnectionInternal* pc) :
		_pc(pc)
	{

	}

	CustomAudioDecoder::~CustomAudioDecoder()
	{

	}

	void CustomAudioDecoder::Reset()
	{
	}

	int CustomAudioDecoder::DecodeInternal(const uint8_t* encoded, size_t encoded_len, int sample_rate_hz, int16_t* decoded, SpeechType* speech_type)
	{
		_pc->onRawAudio(encoded, encoded_len);
		*decoded = 0;
		*speech_type = SpeechType::kSpeech;
		return 0;
	}

	int CustomAudioDecoder::ErrorCode()
	{
		return 0;
	}

	int CustomAudioDecoder::SampleRateHz() const
	{
		return 48000;
	}

	size_t CustomAudioDecoder::Channels() const
	{
		return 2;
	}

}
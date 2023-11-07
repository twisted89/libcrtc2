#include "customvideodecoder.h"
#include "rtcpeerconnection.h"

namespace crtc {
	CustomVideoDecoder::CustomVideoDecoder(RTCPeerConnectionInternal* pc) :
		_decoderInfo(), _pc(pc)
	{
		_decoderInfo.implementation_name = "Custom Decoder";
		_decoderInfo.is_hardware_accelerated = true;
	}

	CustomVideoDecoder::~CustomVideoDecoder()
	{

	}

	bool CustomVideoDecoder::Configure(const Settings& settings)
	{
		return true;
	}
	int32_t CustomVideoDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback)
	{
		return 0;
	}

	int32_t CustomVideoDecoder::Release()
	{
		return 0;
	}

	int32_t CustomVideoDecoder::Decode(const webrtc::EncodedImage& input_image, int64_t render_time_ms)
	{
		_pc->onRawVideo(input_image, render_time_ms);
		return 0;
	}

	int32_t CustomVideoDecoder::Decode(const webrtc::EncodedImage& input_image, bool missing_frames, int64_t render_time_ms)
	{
		(void)missing_frames;
		_pc->onRawVideo(input_image, render_time_ms);
		return 0;
	}

	webrtc::VideoDecoder::DecoderInfo CustomVideoDecoder::GetDecoderInfo() const
	{
		return _decoderInfo;
	}
}
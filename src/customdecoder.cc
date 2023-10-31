#include "customdecoder.h"
#include "rtcpeerconnection.h"

namespace crtc {
	CustomDecoder::CustomDecoder(RTCPeerConnectionInternal* pc) : 
		_decoderInfo(), _pc(pc)
	{
		_decoderInfo.implementation_name = "Custom Decoder";
		_decoderInfo.is_hardware_accelerated = true;
	}

	CustomDecoder::~CustomDecoder()
	{

	}

	bool CustomDecoder::Configure(const Settings& settings)
	{
		return false;
	}
	int32_t CustomDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback)
	{
		return 0;
	}

	int32_t CustomDecoder::Release()
	{
		return 0;
	}

	int32_t CustomDecoder::Decode(const webrtc::EncodedImage& input_image, int64_t render_time_ms)
	{
		_pc->onRawVideo(input_image, render_time_ms);
		return 0;
	}

	webrtc::VideoDecoder::DecoderInfo CustomDecoder::GetDecoderInfo() const
	{
		return _decoderInfo;
	}
}
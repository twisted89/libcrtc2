#include "customdecoderfactory.h"

CustomDecoderFactory::CustomDecoderFactory()
{
}

CustomDecoderFactory::~CustomDecoderFactory()
{
}

std::vector<webrtc::SdpVideoFormat> CustomDecoderFactory::GetSupportedFormats() const
{
	return std::vector<webrtc::SdpVideoFormat>();
}

std::unique_ptr<webrtc::VideoDecoder> CustomDecoderFactory::CreateVideoDecoder(const webrtc::SdpVideoFormat& format)
{
	return std::unique_ptr<webrtc::VideoDecoder>();
}

#ifndef CRTC_CUSTOMDECODERFACTORY_H
#define CRTC_CUSTOMDECODERFACTORY_H

#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include <modules/video_coding/codecs/h264/include/h264.h>

class CustomDecoderFactory : public webrtc::VideoDecoderFactory {
public:
	CustomDecoderFactory();
	virtual ~CustomDecoderFactory();

	// Returns a list of supported video formats in order of preference, to use
	// for signaling etc.
	virtual std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

	// Creates a VideoDecoder for the specified format.
	virtual std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat& format) override;
};

#endif
#ifndef CRTC_CUSTOMVIDEODECODER_H
#define CRTC_CUSTOMVIDEODECODER_H

#include "api/video_codecs/video_decoder.h"

namespace crtc {
	class RTCPeerConnectionInternal;
	class CustomVideoDecoder : public webrtc::VideoDecoder {
	public:
		CustomVideoDecoder(RTCPeerConnectionInternal* pc);
		virtual ~CustomVideoDecoder();

		bool Configure(const Settings& settings) override;
		int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;
		int32_t Release() override;

		int32_t Decode(const webrtc::EncodedImage& input_image, int64_t render_time_ms) override;
		int32_t Decode(const webrtc::EncodedImage& input_image, bool missing_frames, int64_t render_time_ms) override;
		virtual webrtc::VideoDecoder::DecoderInfo GetDecoderInfo() const override;

	private:
		DecoderInfo _decoderInfo;
		RTCPeerConnectionInternal* _pc;
	};

}

#endif
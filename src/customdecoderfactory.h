#ifndef CRTC_CUSTOMDECODERFACTORY_H
#define CRTC_CUSTOMDECODERFACTORY_H

#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include <modules/video_coding/codecs/h264/include/h264.h>

namespace crtc {
	class RTCPeerConnectionInternal;
	class CustomDecoderFactory : public webrtc::VideoDecoderFactory {
	public:
		CustomDecoderFactory(RTCPeerConnectionInternal* pc);
		virtual ~CustomDecoderFactory();

		// Returns a list of supported video formats in order of preference, to use
		// for signaling etc.
		virtual std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

		// Creates a VideoDecoder for the specified format.
		virtual std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat& format) override;

	private:
		bool IsFormatInList(
			const webrtc::SdpVideoFormat& format,
			rtc::ArrayView<const webrtc::SdpVideoFormat> supported_formats) const {
			return absl::c_any_of(
				supported_formats, [&](const webrtc::SdpVideoFormat& supported_format) {
					return supported_format.name == format.name &&
						supported_format.parameters == format.parameters;
				});
		}

		template <typename V, typename... Vs>
		std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoderInternal(
			const webrtc::SdpVideoFormat& format) {
			if (IsFormatInList(format, V::SupportedFormats())) {
				return V::CreateDecoder(format);
			}

			if constexpr (sizeof...(Vs) > 0) {
				return CreateVideoDecoderInternal<Vs...>(format);
			}

			return nullptr;
		}

		RTCPeerConnectionInternal* _pc;
	};

}

#endif
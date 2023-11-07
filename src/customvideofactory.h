#ifndef CRTC_CUSTOMVIDEOFACTORY_H
#define CRTC_CUSTOMVIDEOFACTORY_H

#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include <modules/video_coding/codecs/h264/include/h264.h>

namespace crtc {
	class RTCPeerConnectionInternal;
	class CustomVideoFactory : public webrtc::VideoDecoderFactory {
	public:
		CustomVideoFactory(RTCPeerConnectionInternal* pc);
		virtual ~CustomVideoFactory();

		// Returns a list of supported video formats in order of preference, to use
		// for signaling etc.
		std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

		// Creates a VideoDecoder for the specified format.
		std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(const webrtc::SdpVideoFormat& format) override;

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

		template <typename V, typename... Vs>
		std::vector<webrtc::SdpVideoFormat> GetSupportedFormatsInternal() const {
			auto supported_formats = V::SupportedFormats();

			if constexpr (sizeof...(Vs) > 0) {
				// Supported formats may overlap between implementations, so duplicates
				// should be filtered out.
				for (const auto& other_format : GetSupportedFormatsInternal<Vs...>()) {
					if (!IsFormatInList(other_format, supported_formats)) {
						supported_formats.push_back(other_format);
					}
				}
			}

			return supported_formats;
		}

		RTCPeerConnectionInternal* _pc;
	};

}

#endif
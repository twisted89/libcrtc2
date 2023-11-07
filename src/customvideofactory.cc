#include "customvideofactory.h"
#include "rtcpeerconnection.h"
#include "customvideodecoder.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template_open_h264_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_dav1d_adapter.h"


namespace crtc {

	CustomVideoFactory::CustomVideoFactory(RTCPeerConnectionInternal* pc) :
		_pc(pc)
	{
	}

	CustomVideoFactory::~CustomVideoFactory()
	{
	}

	std::vector<webrtc::SdpVideoFormat> CustomVideoFactory::GetSupportedFormats() const
	{
		return GetSupportedFormatsInternal<
			webrtc::LibvpxVp8DecoderTemplateAdapter,
			webrtc::LibvpxVp9DecoderTemplateAdapter,
			webrtc::OpenH264DecoderTemplateAdapter,
			webrtc::Dav1dDecoderTemplateAdapter>();
	}

	std::unique_ptr<webrtc::VideoDecoder> CustomVideoFactory::CreateVideoDecoder(const webrtc::SdpVideoFormat& format)
	{
		if (_pc->BypassVideoDecoder())
			return std::make_unique<CustomVideoDecoder>(_pc);
		
		return CreateVideoDecoderInternal<
			webrtc::LibvpxVp8DecoderTemplateAdapter,
			webrtc::LibvpxVp9DecoderTemplateAdapter,
			webrtc::OpenH264DecoderTemplateAdapter,
			webrtc::Dav1dDecoderTemplateAdapter>(format);
		
	}
}

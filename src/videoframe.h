#ifndef CRTC_VIDEOFRAME_H
#define CRTC_VIDEOFRAME_H

#include "crtc.h"
#include <api/video/video_frame.h>

namespace crtc {
	class VideoFrameInternal : public VideoFrame {
	public:
		VideoFrameInternal(const webrtc::VideoFrame& frame);
		virtual ~VideoFrameInternal() override;

		virtual uint8_t* Data() override;
		virtual const uint8_t* Data() const override;
		virtual size_t ByteLength() const override;

	protected:
		rtc::scoped_refptr<webrtc::VideoFrameBuffer> _frame;
		rtc::scoped_refptr<webrtc::I420BufferInterface> _420Frame;
	};
}


#endif
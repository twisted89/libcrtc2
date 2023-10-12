#ifndef CRTC_VIDEOFRAME_H
#define CRTC_VIDEOFRAME_H

#include "crtc.h"
#include <api/video/video_frame.h>

namespace crtc {
	class VideoFrameInternal : public VideoFrame {
	public:
		static std::shared_ptr<VideoFrame> New(const webrtc::VideoFrame &frame);

	protected:
		explicit VideoFrameInternal(const webrtc::VideoFrame &frame);
		~VideoFrameInternal() override;

		rtc::scoped_refptr<webrtc::VideoFrameBuffer> _frame;
	};
}


#endif
#include "videoframe.h"

using namespace crtc;

std::shared_ptr<VideoFrame> VideoFrameInternal::New(const webrtc::VideoFrame &frame) {
	return std::make_shared<VideoFrameInternal>(frame);
}

VideoFrameInternal::VideoFrameInternal(const webrtc::VideoFrame &frame) : _frame(frame.video_frame_buffer())
{

}

VideoFrameInternal::~VideoFrameInternal()
{

}
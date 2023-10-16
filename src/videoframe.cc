#include "videoframe.h"

using namespace crtc;

uint32_t VideoFrame::TimeStamp() const 
{ 
	return _timestamp; 
}

VideoFrameInternal::VideoFrameInternal(const webrtc::VideoFrame &frame) : _frame(frame.video_frame_buffer())
{
	_420Frame = _frame->ToI420();
	_timestamp = frame.timestamp();
}

VideoFrameInternal::~VideoFrameInternal()
{

}

uint8_t* VideoFrameInternal::Data()
{
	return const_cast<uint8_t*>(_420Frame->DataY());
}

const uint8_t* VideoFrameInternal::Data() const
{
	return _420Frame->DataY();
}

size_t VideoFrameInternal::ByteLength() const
{
	return _420Frame->ChromaWidth() * _420Frame->ChromaHeight();
}
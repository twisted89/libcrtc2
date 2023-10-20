/*
* The MIT License (MIT)
*
* Copyright (c) 2017 vmolsa <ville.molsa@gmail.com> (http://github.com/vmolsa)
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*/

#include "crtc.h"
#include "imagebuffer.h"
#include <api/make_ref_counted.h>
#include <api/video/i420_buffer.h>
#include <third_party/libyuv/include/libyuv/convert.h>
#include <third_party/libyuv/include/libyuv/scale.h>

using namespace crtc;

ImageBufferInternal::ImageBufferInternal(const std::shared_ptr<ArrayBuffer>& buffer, int width, int height) :
	ArrayBufferInternal(buffer),
	_width(width),
	_height(height)
{
	_y = ArrayBufferInternal::Data();
	_u = ArrayBufferInternal::Data() + _width * _height;
	_v = ArrayBufferInternal::Data() + _width * _height + ((_width + 1) >> 1) * ((_height + 1) >> 1);
}

ImageBufferInternal::ImageBufferInternal(int width, int height) :
	ArrayBufferInternal(nullptr, ImageBuffer::ByteLength(width, height)),
	_width(width),
	_height(height)
{
	_y = ArrayBufferInternal::Data();
	_u = ArrayBufferInternal::Data() + _width * _height;
	_v = ArrayBufferInternal::Data() + _width * _height + ((_width + 1) >> 1) * ((_height + 1) >> 1);
}

ImageBufferInternal::~ImageBufferInternal() {

}

std::shared_ptr<ImageBuffer> ImageBufferInternal::New(const std::shared_ptr<ArrayBuffer>& buffer, int width, int height) {
	return std::make_shared<ImageBufferInternal>(buffer, width, height);
}

std::shared_ptr<ImageBuffer> ImageBufferInternal::New(int width, int height) {
	return std::make_shared<ImageBufferInternal>(width, height);
}

int ImageBufferInternal::Width() const {
	return _width;
}

int ImageBufferInternal::Height() const {
	return _height;
}

const uint8_t* ImageBufferInternal::DataY() const {
	return _y;
}

const uint8_t* ImageBufferInternal::DataU() const {
	return _u;
}

const uint8_t* ImageBufferInternal::DataV() const {
	return _v;
}

int ImageBufferInternal::StrideY() const {
	return _width;
}

int ImageBufferInternal::StrideU() const {
	return (_width + 1) >> 1;
}

int ImageBufferInternal::StrideV() const {
	return (_width + 1) >> 1;
}

size_t ImageBufferInternal::ByteLength() const {
	return ArrayBufferInternal::ByteLength();
}

std::shared_ptr<ArrayBuffer> ImageBufferInternal::Slice(size_t begin, size_t end) const {
	return ArrayBufferInternal::Slice(begin, end);
}

uint8_t* ImageBufferInternal::Data() {
	return ArrayBufferInternal::Data();
}

const uint8_t* ImageBufferInternal::Data() const {
	return ArrayBufferInternal::Data();
}

String ImageBufferInternal::ToString() const {
	return ArrayBufferInternal::ToString();
}

std::shared_ptr<ImageBuffer> ImageBuffer::New(int width, int height) {
	return ImageBufferInternal::New(width, height);
}

std::shared_ptr<ImageBuffer> ImageBuffer::New(const std::shared_ptr<ArrayBuffer>& buffer, int width, int height) {
	if (ImageBuffer::ByteLength(width, height) == buffer->ByteLength()) {
		return ImageBufferInternal::New(buffer, width, height);
	}

	return nullptr;
}

size_t ImageBuffer::ByteLength(int height, int stride_y, int stride_u, int stride_v) {
	return static_cast<size_t>(stride_y * height + (stride_u + stride_v) * ((height + 1) >> 1));
}

size_t ImageBuffer::ByteLength(int width, int height) {
	if (width > 0 && height > 0) {
		return ByteLength(height, width, (width + 1) >> 1, (width + 1) >> 1);
	}

	return 0;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> WrapImageBuffer::New(const std::shared_ptr<ImageBuffer>& source) {
	if (source) {
		return rtc::make_ref_counted<WrapImageBuffer>(source);
	}

	return nullptr;
}

WrapImageBuffer::WrapImageBuffer(const std::shared_ptr<ImageBuffer>& source) :
	_source(source)
{ }

WrapImageBuffer::~WrapImageBuffer() {

}

int WrapImageBuffer::width() const {
	return _source->Width();
}

int WrapImageBuffer::height() const {
	return _source->Height();
}

webrtc::VideoFrameBuffer::Type WrapImageBuffer::type() const
{
	return webrtc::VideoFrameBuffer::Type::kI420;
}

const webrtc::I420BufferInterface* WrapImageBuffer::GetI420() const
{
	return nullptr;
}

rtc::scoped_refptr<webrtc::I420BufferInterface> WrapImageBuffer::ToI420() {
	rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer = webrtc::I420Buffer::Create(width(), height());
	int res = libyuv::I420ToI420(
		_source->DataY(), _source->StrideY(), _source->DataU(), _source->StrideU(), _source->DataV(), _source->StrideV(),
		i420_buffer->MutableDataY(), i420_buffer->StrideY(),
		i420_buffer->MutableDataU(), i420_buffer->StrideU(),
		i420_buffer->MutableDataV(), i420_buffer->StrideV(), width(), height());
	RTC_DCHECK_EQ(res, 0);

	return i420_buffer;
}

std::shared_ptr<ImageBuffer> WrapVideoFrameBuffer::New(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& vfb) {
	if (vfb.get()) {
		return std::make_shared<WrapVideoFrameBuffer>(vfb);
	}

	return nullptr;
}

WrapVideoFrameBuffer::WrapVideoFrameBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& vfb) :
	_vfb(vfb)
{ }

WrapVideoFrameBuffer::~WrapVideoFrameBuffer() {

}

int WrapVideoFrameBuffer::Width() const {
	return _vfb->width();
}

int WrapVideoFrameBuffer::Height() const {
	return _vfb->height();
}

const webrtc::I420BufferInterface* WrapVideoFrameBuffer::GetI420() const
{
	return _vfb->GetI420();
}

rtc::scoped_refptr<webrtc::I420BufferInterface> WrapVideoFrameBuffer::ToI420() {
	return _vfb->ToI420();
}

size_t WrapVideoFrameBuffer::ByteLength() const {
	auto i420 = _vfb->GetI420();
	return ImageBuffer::ByteLength(_vfb->height(), i420->StrideY(), i420->StrideU(), i420->StrideV());
}

std::shared_ptr<ArrayBuffer> WrapVideoFrameBuffer::Slice(size_t begin, size_t end) const {
	auto buffer = ArrayBuffer::New(Data(), ByteLength());

	if (buffer) {
		return buffer->Slice(begin, end);
	}

	return nullptr;
}

uint8_t* WrapVideoFrameBuffer::Data() {
	return const_cast<uint8_t*>(_vfb->GetI420()->DataY());
}

const uint8_t* WrapVideoFrameBuffer::Data() const {
	return _vfb->GetI420()->DataY();
}

String WrapVideoFrameBuffer::ToString() const {
	return String(reinterpret_cast<const char*>(Data()), ByteLength());
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> WrapBufferToVideoFrameBuffer::New(const std::shared_ptr<ArrayBuffer>& source, int width, int height) {
	if (source) {
		return rtc::make_ref_counted<WrapBufferToVideoFrameBuffer>(source, width, height);
	}

	return nullptr;
}

WrapBufferToVideoFrameBuffer::WrapBufferToVideoFrameBuffer(const std::shared_ptr<ArrayBuffer>& source, int width, int height) :
	_source(source),
	_width(width),
	_height(height),
	_y(nullptr),
	_u(nullptr),
	_v(nullptr)
{
	_y = source->Data();
	_u = source->Data() + _width * _height;
	_v = source->Data() + _width * _height + ((_width + 1) >> 1) * ((_height + 1) >> 1);
}

WrapBufferToVideoFrameBuffer::~WrapBufferToVideoFrameBuffer() {

}

int WrapBufferToVideoFrameBuffer::width() const {
	return _width;
}

int WrapBufferToVideoFrameBuffer::height() const {
	return _height;
}

const uint8_t* WrapBufferToVideoFrameBuffer::DataY() const {
	return _y;
}

const uint8_t* WrapBufferToVideoFrameBuffer::DataU() const {
	return _u;
}

const uint8_t* WrapBufferToVideoFrameBuffer::DataV() const {
	return _v;
}

int WrapBufferToVideoFrameBuffer::StrideY() const {
	return _width;
}

int WrapBufferToVideoFrameBuffer::StrideU() const {
	return (_width + 1) >> 1;
}

int WrapBufferToVideoFrameBuffer::StrideV() const {
	return (_width + 1) >> 1;
}

const webrtc::I420BufferInterface* WrapBufferToVideoFrameBuffer::GetI420() const
{
	return nullptr;
}

rtc::scoped_refptr<webrtc::I420BufferInterface> WrapBufferToVideoFrameBuffer::ToI420() {
	rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer = webrtc::I420Buffer::Create(width(), height());
	int res = libyuv::I420ToI420(
		DataY(), StrideY(), DataU(), StrideU(), DataV(), StrideV(),
		i420_buffer->MutableDataY(), i420_buffer->StrideY(),
		i420_buffer->MutableDataU(), i420_buffer->StrideU(),
		i420_buffer->MutableDataV(), i420_buffer->StrideV(), width(), height());
	RTC_DCHECK_EQ(res, 0);

	return i420_buffer;
}

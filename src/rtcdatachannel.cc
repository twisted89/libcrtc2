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
#include "rtcdatachannel.h"
#include "arraybuffer.h"

using namespace crtc;

RTCDataChannelInternal::RTCDataChannelInternal(const rtc::scoped_refptr<webrtc::DataChannelInterface>& channel) :
	_threshold(0),
	_channel(channel)
{
	_channel->RegisterObserver(this);

	if (_channel->state() == webrtc::DataChannelInterface::kOpen ||
		_channel->state() == webrtc::DataChannelInterface::kConnecting)
	{
		_event = Event::New();
	}
}

RTCDataChannelInternal::~RTCDataChannelInternal() {
	_channel->UnregisterObserver();
}

int RTCDataChannelInternal::Id() {
	return _channel->id();
}

String RTCDataChannelInternal::Label() {
	return String(_channel->label().c_str());
}

uint64_t RTCDataChannelInternal::BufferedAmount() {
	return _channel->buffered_amount();
}

uint64_t RTCDataChannelInternal::BufferedAmountLowThreshold() {
	return _threshold;
}

void RTCDataChannelInternal::SetBufferedAmountLowThreshold(uint64_t threshold) {
	_threshold = threshold;
}

uint16_t RTCDataChannelInternal::MaxPacketLifeTime() {
	return _channel->maxRetransmitTime();
}

uint16_t RTCDataChannelInternal::MaxRetransmits() {
	return _channel->maxRetransmits();
}

bool RTCDataChannelInternal::Negotiated() {
	return _channel->negotiated();
}

bool RTCDataChannelInternal::Ordered() {
	return _channel->ordered();
}

String RTCDataChannelInternal::Protocol() {
	return String(_channel->protocol().c_str());
}

RTCDataChannel::State RTCDataChannelInternal::ReadyState() {
	switch (_channel->state()) {
	case webrtc::DataChannelInterface::kConnecting:
		return RTCDataChannel::State::kConnecting;
	case webrtc::DataChannelInterface::kOpen:
		return RTCDataChannel::State::kOpen;
	case webrtc::DataChannelInterface::kClosing:
		return RTCDataChannel::State::kClosing;
	case webrtc::DataChannelInterface::kClosed:
	default:
		return RTCDataChannel::State::kClosed;
	}
}

void RTCDataChannelInternal::Close() {
	_channel->Close();
}

void RTCDataChannelInternal::Send(const std::shared_ptr<ArrayBuffer>& data, bool binary) {
	rtc::CopyOnWriteBuffer buffer(data->Data(), data->ByteLength());
	webrtc::DataBuffer dataBuffer(buffer, binary);

	if (!_channel->Send(dataBuffer)) {
		switch (_channel->state()) {
		case webrtc::DataChannelInterface::kConnecting:
			_onerror(Error::New("Unable to send arraybuffer. DataChannel is connecting", __FILE__, __LINE__));
			break;
		case webrtc::DataChannelInterface::kOpen:
			_onerror(Error::New("Unable to send arraybuffer.", __FILE__, __LINE__));
			break;
		case webrtc::DataChannelInterface::kClosing:
			_onerror(Error::New("Unable to send arraybuffer. DataChannel is closing", __FILE__, __LINE__));
			break;
		case webrtc::DataChannelInterface::kClosed:
			_onerror(Error::New("Unable to send arraybuffer. DataChannel is closed", __FILE__, __LINE__));
			break;
		}
	}
}

void RTCDataChannelInternal::Send(const unsigned char* data, size_t length, bool binary) {
	rtc::CopyOnWriteBuffer buffer(data, length);
	webrtc::DataBuffer dataBuffer(buffer, binary);

	if (!_channel->Send(dataBuffer)) {
		switch (_channel->state()) {
		case webrtc::DataChannelInterface::kConnecting:
			_onerror(Error::New("Unable to send arraybuffer. DataChannel is connecting", __FILE__, __LINE__));
			break;
		case webrtc::DataChannelInterface::kOpen:
			_onerror(Error::New("Unable to send arraybuffer.", __FILE__, __LINE__));
			break;
		case webrtc::DataChannelInterface::kClosing:
			_onerror(Error::New("Unable to send arraybuffer. DataChannel is closing", __FILE__, __LINE__));
			break;
		case webrtc::DataChannelInterface::kClosed:
			_onerror(Error::New("Unable to send arraybuffer. DataChannel is closed", __FILE__, __LINE__));
			break;
		}
	}
}

void crtc::RTCDataChannelInternal::onBufferedAmountLow(std::function<void()> callback)
{
	_onbufferedamountlow = callback;
}

void crtc::RTCDataChannelInternal::onOpen(std::function<void()> callback)
{
	_onopen = callback;
}

void crtc::RTCDataChannelInternal::onClose(std::function<void()> callback)
{
	_onclose = callback;
}

void crtc::RTCDataChannelInternal::onMessage(std::function<void(std::shared_ptr<ArrayBuffer>, bool)> callback)
{
	_onmessage = callback;
}

void crtc::RTCDataChannelInternal::onError(std::function<void(std::shared_ptr<Error>)> callback)
{
	_onerror = callback;
}

void RTCDataChannelInternal::OnStateChange() {
	switch (_channel->state()) {
	case webrtc::DataChannelInterface::kConnecting:
		break;
	case webrtc::DataChannelInterface::kOpen:
		_onopen();
		break;
	case webrtc::DataChannelInterface::kClosing:
		break;
	case webrtc::DataChannelInterface::kClosed:
		_onclose();
		_event.reset();
		break;
	}
}

void RTCDataChannelInternal::OnMessage(const webrtc::DataBuffer& buffer) {
	_onmessage(WrapRtcBuffer::New(buffer.data.data(), buffer.size()), buffer.binary);
}

void RTCDataChannelInternal::OnBufferedAmountChange(uint64_t previous_amount) {
	if (_threshold && previous_amount > _threshold && _channel->buffered_amount() < _threshold) {
		_onbufferedamountlow();
	}
}

WrapRtcBuffer::WrapRtcBuffer(const rtc::CopyOnWriteBuffer& buffer) : _data(buffer) {

}

WrapRtcBuffer::~WrapRtcBuffer() {

}

size_t WrapRtcBuffer::ByteLength() const {
	return _data.size();
}

std::shared_ptr<ArrayBuffer> WrapRtcBuffer::Slice(size_t begin, size_t end) const {
	if (begin <= end && end <= _data.size()) {
		return std::make_shared<ArrayBufferInternal>(_data.data() + begin, ((!end) ? _data.size() : end - begin));
	}

	return nullptr;
}

uint8_t* WrapRtcBuffer::Data() {
	return const_cast<uint8_t*>(_data.data());
}

const uint8_t* WrapRtcBuffer::Data() const {
	return _data.data();
}

String WrapRtcBuffer::ToString() const {
	return String(reinterpret_cast<const char*>(_data.data()), _data.size());
}

RTCDataChannel::RTCDataChannel() {

}

RTCDataChannel::~RTCDataChannel() {

}
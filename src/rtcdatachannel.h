
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

#ifndef CRTC_RTCDATACHANNEL_H
#define CRTC_RTCDATACHANNEL_H

#include "crtc.h"
#include "event.h"
#include "utils.hpp"
#include <api/data_channel_interface.h>

namespace crtc {
	class RTCDataChannelInternal : public RTCDataChannel, public webrtc::DataChannelObserver {
	public:
		explicit RTCDataChannelInternal(const rtc::scoped_refptr<webrtc::DataChannelInterface>& channel);
		virtual ~RTCDataChannelInternal() override;

		int Id() override;
		String Label() override;
		uint64_t BufferedAmount() override;
		uint64_t BufferedAmountLowThreshold() override;
		void SetBufferedAmountLowThreshold(uint64_t threshold = 0) override;
		uint16_t MaxPacketLifeTime() override;
		uint16_t MaxRetransmits() override;
		bool Negotiated() override;
		bool Ordered() override;
		String Protocol() override;
		RTCDataChannel::State ReadyState() override;
		void Close() override;
		void Send(const std::shared_ptr<ArrayBuffer>& data, bool binary = true) override;
		void Send(const unsigned char* data, size_t length, bool binary = true) override;

		void onBufferedAmountLow(std::function<void()> callback) override;
		void onOpen(std::function<void()> callback) override;
		void onClose(std::function<void()> callback) override;
		void onMessage(std::function<void(std::shared_ptr<ArrayBuffer>, bool)> callback) override;
		void onError(std::function<void(std::shared_ptr<Error>)> callback) override;

	protected:
		void OnStateChange() override;
		void OnMessage(const webrtc::DataBuffer& buffer) override;
		void OnBufferedAmountChange(uint64_t previous_amount) override;

		uint64_t _threshold;
		std::shared_ptr<Event> _event;
		rtc::scoped_refptr<webrtc::DataChannelInterface> _channel;

		synchronized_callback<> _onbufferedamountlow;
		synchronized_callback<> _onclose;
		synchronized_callback<std::shared_ptr<Error>> _onerror;
		synchronized_callback<std::shared_ptr<ArrayBuffer>, bool> _onmessage;
		synchronized_callback<> _onopen;
	};

	class WrapRtcBuffer : public ArrayBuffer {

	public:
		size_t ByteLength() const override;

		std::shared_ptr<ArrayBuffer> Slice(size_t begin = 0, size_t end = 0) const override;

		uint8_t* Data() override;
		const uint8_t* Data() const override;

		String ToString() const override;
	protected:
		explicit WrapRtcBuffer(const rtc::CopyOnWriteBuffer& buffer);
		~WrapRtcBuffer();

		rtc::CopyOnWriteBuffer _data;
	};
}

#endif
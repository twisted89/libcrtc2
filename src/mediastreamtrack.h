
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

#ifndef CRTC_MEDIASTREAMTRACK_H
#define CRTC_MEDIASTREAMTRACK_H

#include "crtc.h"
#include "utils.hpp"
#include <api/media_stream_interface.h>

namespace crtc {
	class MediaStreamTrackInternal : public MediaStreamTrack, public webrtc::ObserverInterface, 
		webrtc::AudioTrackSinkInterface, rtc::VideoSinkInterface<webrtc::VideoFrame> {

	public:
		MediaStreamTrackInternal(webrtc::MediaStreamTrackInterface* track);
		virtual ~MediaStreamTrackInternal() override;

		bool Enabled() const override;
		bool Remote() const override;
		bool Muted() const override;
		String Id() const override;
		std::string IdString() const;
		MediaStreamTrack::Type Kind() const override;
		MediaStreamTrack::State ReadyState() const override;

		void onStarted(std::function<void()> callback) override;
		void onEnded(std::function<void()> callback) override;
		void onMute(std::function<void()> callback) override;
		void onUnmute(std::function<void()> callback) override;
		void onAudio(std::function<void(const void*, int, int, size_t, size_t)> callback) override;
		void onVideo(std::function<void(std::shared_ptr<VideoFrame>)> callback) override;
		void onFrameDrop(std::function<void()> callback) override;

		rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> GetTrack() const;
		webrtc::MediaSourceInterface* GetSource() const;

		void ClearObserver();

	private:
		void OnChanged() override;

	protected:
		virtual void OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) override;
		virtual void OnFrame(const webrtc::VideoFrame& frame) override;
		virtual void OnDiscardedFrame() override;
		virtual void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) override;
		virtual void OnStarted();
		virtual void OnUnMute();
		virtual void OnMute();
		virtual void OnEnded();

		MediaStreamTrack::Type _kind;
		rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> _track;
		//rtc::scoped_refptr<webrtc::MediaSourceInterface> _source;
		webrtc::MediaSourceInterface::SourceState _state;

		synchronized_callback<> _onstarted;
		synchronized_callback<> _onended;
		synchronized_callback<> _onmute;
		synchronized_callback<> _onunmute;

		synchronized_callback<const void*, int, int, size_t, size_t> _onAudio;
		synchronized_callback<std::shared_ptr<VideoFrame>> _onVideo;
		synchronized_callback<> _onFrameDrop;
	};
}

#endif
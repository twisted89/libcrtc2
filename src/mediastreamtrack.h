
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
#include <api/media_stream_interface.h>

namespace crtc {
	class MediaStreamTrackInternal : public MediaStreamTrack, public webrtc::ObserverInterface, 
		webrtc::AudioTrackSinkInterface, rtc::VideoSinkInterface<webrtc::VideoFrame> {
		friend class Let<MediaStreamTrackInternal>;

	public:
		static webrtc::MediaStreamTrackInterface* New(const Let<MediaStreamTrack>& track);
		static Let<MediaStreamTrack> New(webrtc::MediaStreamTrackInterface* track = nullptr);

		virtual void OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) override;
		virtual void OnFrame(const webrtc::VideoFrame& frame) override;

		// Should be called by the source when it discards the frame due to rate
		// limiting.
		virtual void OnDiscardedFrame() override;

		// Called on the network thread when video constraints change.
		// TODO(crbug/1255737): make pure virtual once downstream project adapts.
		virtual void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) override;

		bool Enabled() const override;
		bool Remote() const override;
		bool Muted() const override;
		std::string Id() const override;
		MediaStreamTrack::Type Kind() const override;
		MediaStreamTrack::State ReadyState() const override;
		Let<MediaStreamTrack> Clone() override;

		rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> GetTrack() const;
		rtc::scoped_refptr<webrtc::MediaSourceInterface> GetSource() const;

	private:
		void OnChanged() override;

	protected:
		MediaStreamTrackInternal(MediaStreamTrack::Type kind, webrtc::MediaStreamTrackInterface* track = nullptr, webrtc::MediaSourceInterface* source = nullptr);
		MediaStreamTrackInternal(const Let<MediaStreamTrackInternal>& track);

		virtual void OnStarted() {
			onstarted();
		}

		virtual void OnUnMute() {
			onunmute();
		}

		virtual void OnMute() {
			onmute();
		}

		virtual void OnEnded() {
			onended();
		}

		~MediaStreamTrackInternal() override;

		MediaStreamTrack::Type _kind;
		rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> _track;
		rtc::scoped_refptr<webrtc::MediaSourceInterface> _source;
		webrtc::MediaSourceInterface::SourceState _state;
	};
}

#endif
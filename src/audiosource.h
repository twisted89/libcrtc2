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

#ifndef CRTC_AUDIOSOURCE_H
#define CRTC_AUDIOSOURCE_H

#include "crtc.h"
#include "mediastream.h"
#include "audiodevice.h"
#include "utils.hpp"

namespace crtc {
	class AudioSourceInternal : public AudioSource, public MediaStreamInternal, public sigslot::has_slots<> {
		friend class AudioSource;
	public:
		explicit AudioSourceInternal();
		virtual ~AudioSourceInternal() override;

		bool IsRunning() const override;
		void Stop() override;

		void Write(const std::shared_ptr<AudioBuffer>& buffer, std::function<void(std::shared_ptr<Error>)> callback) override;

		String Id() const override;
		void AddTrack(const std::shared_ptr<MediaStreamTrack>& track) override;
		void RemoveTrack(const std::shared_ptr<MediaStreamTrack>& track) override;
		std::shared_ptr<MediaStreamTrack> GetTrackById(const String& id) const override;
		intptr_t GetStream() override;
		MediaStreamTracks GetAudioTracks() const override;
		MediaStreamTracks GetVideoTracks() const override;
		std::shared_ptr<MediaStream> Clone() override;

	protected:
		void OnDrain();

		static volatile int counter;
		rtc::scoped_refptr<AudioDevice> _audio;
		synchronized_callback<> _ondrain;
	};
}

#endif
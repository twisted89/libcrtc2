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

#ifndef CRTC_AUDIODEVICE_H
#define CRTC_AUDIODEVICE_H

#include "crtc.h"
#include "worker.h"
#include "modules/audio_device/include/fake_audio_device.h"
#include <rtc_base/synchronization/mutex.h>
#include <rtc_base/thread.h>
#include "rtc_base/time_utils.h"
#include "rtc_base/timestamp_aligner.h"

namespace crtc {
	class AudioDevice : public webrtc::FakeAudioDeviceModule {
	public:
		AudioDevice() :
			_capturing(false),
			_drainNeeded(false),
			_clock(RealTimeClock::New(Functor<void()>(this, &AudioDevice::OnTime)))
		{

		}

		~AudioDevice() override {
			StopRecording();
			_clock->Stop();
		}

		sigslot::signal0<> Drain;

		inline void Write(const Let<AudioBuffer>& buffer, ErrorCallback callback) {
			webrtc::MutexLock lock(&lock_);

			if (Recording()) {
				_queue.push_back(Queue(buffer, callback));
			}
			else {
				callback(Error::New("AudioDevice is not recording.", __FILE__, __LINE__));
			}
		}

		inline int32_t Init() override {
			_clock->Start(100); // 100 * 10ms = 1000ms 
			return 0;
		}

		inline int32_t RegisterAudioCallback(webrtc::AudioTransport* callback) override {
			webrtc::MutexLock lock(&lock_);
			_callback = callback;
			return 0;
		}

		inline int32_t StartPlayout() override {
			return -1;
		}

		inline int32_t StopPlayout() override {
			return 0;
		}

		inline int32_t StartRecording() override {
			webrtc::MutexLock lock(&lock_);
			_capturing = true;
			return 0;
		}

		inline int32_t StopRecording() override {
			_capturing = false;
			return 0;
		}

		inline bool Playing() const override {
			return false;
		}

		inline bool Recording() const override {
			return _capturing;
		}

	private:
		class Queue {
		public:
			explicit Queue()
			{ }

			Queue(const Let<AudioBuffer>& audio_buffer, const ErrorCallback& errorCallback) :
				buffer(audio_buffer),
				callback(errorCallback),
				timestamp(rtc::TimeNanos())
			{ }

			Let<AudioBuffer> buffer;
			ErrorCallback callback;
			int64_t timestamp;
		};

		inline void OnTime() {
			if (_capturing) {
				Queue pending;

				{
					webrtc::MutexLock lock(&lock_);

					if (!_queue.empty()) {
						pending = _queue.front();
						_queue.pop_front();
					}
					else {
						if (_drainNeeded) {
							_drainNeeded = false;
							Drain();
						}

						return;
					}
				}

				uint32_t new_mic_level = 0;

				{
					webrtc::MutexLock lock(&lock_);
					_callback->RecordedDataIsAvailable(pending.buffer->Data(), pending.buffer->ByteLength(), pending.buffer->BitsPerSample() / 8, pending.buffer->Channels(), pending.buffer->SampleRate(), 0, 0, 0, false, new_mic_level);
				}

				pending.callback(Let<Error>());

				{
					webrtc::MutexLock lock(&lock_);

					if (!_queue.empty()) {
						_drainNeeded = true;
					}
				}
			}
		}

		webrtc::Mutex lock_;

		bool _capturing;
		bool _drainNeeded;

		std::list<Queue> _queue RTC_GUARDED_BY(lock_);
		webrtc::AudioTransport* _callback RTC_GUARDED_BY(lock_);
		Let<RealTimeClock> _clock;
	};
}

#endif
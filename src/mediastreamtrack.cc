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
#include "mediastreamtrack.h"
#include "rtc_base/logging.h"
#include "videoframe.h"

using namespace crtc;

void MediaStreamTrackInternal::OnChanged() {
	auto source = GetSource();
	if (source)
	{
		switch (source->state()) {
		case webrtc::MediaSourceInterface::kInitializing:
			break;
		case webrtc::MediaSourceInterface::kLive:
			switch (_state) {
			case webrtc::MediaSourceInterface::kInitializing:
				OnStarted();
				break;
			case webrtc::MediaSourceInterface::kLive:
				break;
			case webrtc::MediaSourceInterface::kEnded:
				break;
			case webrtc::MediaSourceInterface::kMuted:
				OnUnMute();
				break;
			}

			break;
		case webrtc::MediaSourceInterface::kEnded:
			OnEnded();
			break;
		case webrtc::MediaSourceInterface::kMuted:
			OnMute();
			break;
		}
		_state = source->state();
	}
}

/*
webrtc::MediaStreamTrackInterface* MediaStreamTrackInternal::New(const std::shared_ptr<MediaStreamTrack>& track) {
	if (track) {
		MediaStreamTrackInternal track_internal(track);
		return track_internal._track.get();
	}

	return nullptr;
}
*/

MediaStreamTrackInternal::MediaStreamTrackInternal(webrtc::MediaStreamTrackInterface* track) :
	_track(track)
{
	_kind = track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind ? MediaStreamTrack::kAudio : MediaStreamTrack::kVideo;

	if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
		_kind = MediaStreamTrack::kAudio;
		webrtc::AudioTrackInterface* audio = static_cast<webrtc::AudioTrackInterface*>(track);
		//Async::Call([=]() {
			audio->GetSource()->RegisterObserver(this);
			_state = audio->GetSource()->state();
		//	});
		audio->AddSink(this);
	}
	else {
		_kind = MediaStreamTrack::kVideo;
		webrtc::VideoTrackInterface* video = static_cast<webrtc::VideoTrackInterface*>(track);

		//Async::Call([=]() {
			video->GetSource()->RegisterObserver(this);
			_state = video->GetSource()->state();
		//});

		rtc::VideoSinkWants wants;
		video->AddOrUpdateSink(this, wants);
		video->set_enabled(true);
		
	}
}

MediaStreamTrackInternal::~MediaStreamTrackInternal() {

	if (_kind == MediaStreamTrack::Type::kAudio) {
		webrtc::AudioTrackInterface* audio = static_cast<webrtc::AudioTrackInterface*>(_track.get());
		audio->RemoveSink(this);
	}
	else if (_kind == MediaStreamTrack::Type::kVideo) {
		webrtc::VideoTrackInterface* video = static_cast<webrtc::VideoTrackInterface*>(_track.get());
		rtc::VideoSinkWants wants;
		video->RemoveSink(this);
	}

}


void MediaStreamTrackInternal::OnData(const void* audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames)
{
	_onAudio(audio_data, bits_per_sample, sample_rate, number_of_channels, number_of_frames);
}

void MediaStreamTrackInternal::OnFrame(const webrtc::VideoFrame& frame) {
	_onVideo(std::make_shared<VideoFrameInternal>(frame));
}

void MediaStreamTrackInternal::OnDiscardedFrame() {
	_onFrameDrop();
}

void MediaStreamTrackInternal::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) {
	(void)constraints;
}

void MediaStreamTrackInternal::OnStarted() {
	_onstarted();
}

void MediaStreamTrackInternal::OnUnMute() {
	_onunmute();
}

void MediaStreamTrackInternal::OnMute() {
	_onmute();
}

void MediaStreamTrackInternal::OnEnded() {
	_onended();
}

bool MediaStreamTrackInternal::Enabled() const {
	return _track->enabled();
}

bool MediaStreamTrackInternal::Remote() const {
	auto source = GetSource();
	return source ? source->remote() : true;
}

bool MediaStreamTrackInternal::Muted() const {
	auto source = GetSource();
	return source ? source->state() == webrtc::MediaSourceInterface::kMuted : false;
}

String MediaStreamTrackInternal::Id() const {
	return String(_track->id().c_str());
}

std::string MediaStreamTrackInternal::IdString() const {
	return _track->id();
}

MediaStreamTrack::Type MediaStreamTrackInternal::Kind() const {
	return _kind;
}

MediaStreamTrack::State MediaStreamTrackInternal::ReadyState() const {
	auto source = GetSource();
	if (source && (_track->state() == webrtc::MediaStreamTrackInterface::kEnded || source->state() == webrtc::MediaSourceInterface::kEnded)) {
		return MediaStreamTrack::kEnded;
	}

	return MediaStreamTrack::kLive;
}

void crtc::MediaStreamTrackInternal::onStarted(std::function<void()> callback)
{
	_onstarted = callback;
}

void crtc::MediaStreamTrackInternal::onEnded(std::function<void()> callback)
{
	_onended = callback;
}

void crtc::MediaStreamTrackInternal::onMute(std::function<void()> callback)
{
	_onmute = callback;
}

void crtc::MediaStreamTrackInternal::onUnmute(std::function<void()> callback)
{
	_onunmute = callback;
}

void crtc::MediaStreamTrackInternal::onAudio(std::function<void(const void*, int, int, size_t, size_t)> callback)
{
	_onAudio = callback;
}

void crtc::MediaStreamTrackInternal::onVideo(std::function<void(std::shared_ptr<VideoFrame>)> callback)
{
	_onVideo = callback;
}

void crtc::MediaStreamTrackInternal::onFrameDrop(std::function<void()> callback)
{
	_onFrameDrop = callback;
}

rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> MediaStreamTrackInternal::GetTrack() const {
	return _track;
}

webrtc::MediaSourceInterface* MediaStreamTrackInternal::GetSource() const {
	if (_kind == MediaStreamTrack::Type::kAudio) {
		return static_cast<webrtc::AudioTrackInterface*>(_track.get())->GetSource();
	}
	else if (_kind == MediaStreamTrack::Type::kVideo) {
		return static_cast<webrtc::VideoTrackInterface*>(_track.get())->GetSource();
	}
	return nullptr;
}

void crtc::MediaStreamTrackInternal::ClearObserver()
{
	auto source = GetSource();
	if(source)
		Async::Call([=]() { source->UnregisterObserver(this); });
}

MediaStreamTrack::MediaStreamTrack() {

}

MediaStreamTrack::~MediaStreamTrack() {

}

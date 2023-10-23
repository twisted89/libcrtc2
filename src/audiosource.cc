
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
#include "audiosource.h"


using namespace crtc;

AudioSourceInternal::AudioSourceInternal() {
  _audio->Drain.connect(this, &AudioSourceInternal::OnDrain);
}

AudioSourceInternal::~AudioSourceInternal() {
  _audio->StopRecording();
  _audio->Drain.disconnect(this);
}

bool AudioSourceInternal::IsRunning() const {
  return _audio->Recording();
}

void AudioSourceInternal::Stop() {
  _audio->StopRecording();
}

void AudioSourceInternal::Write(const std::shared_ptr<AudioBuffer> &buffer, std::function<void(std::shared_ptr<Error>)> callback) {
  _audio->Write(buffer, callback);
}

String AudioSourceInternal::Id() const {
  return MediaStreamInternal::Id();
}


void AudioSourceInternal::AddTrack(const std::shared_ptr<MediaStreamTrack>& track) {
  return MediaStreamInternal::AddTrack(track);
}

void AudioSourceInternal::RemoveTrack(const std::shared_ptr<MediaStreamTrack>& track) {
  return MediaStreamInternal::RemoveTrack(track);
}


std::shared_ptr<MediaStreamTrack> AudioSourceInternal::GetTrackById(const String&id) const {
  return MediaStreamInternal::GetTrackById(id);
}

intptr_t AudioSourceInternal::GetStream()
{
	return MediaStreamInternal::GetStream();
}

MediaStreamTracks AudioSourceInternal::GetAudioTracks() const { 
  return MediaStreamInternal::GetAudioTracks();
}

MediaStreamTracks AudioSourceInternal::GetVideoTracks() const {
  return MediaStreamInternal::GetVideoTracks();
}

std::shared_ptr<MediaStream> AudioSourceInternal::Clone() {
  return MediaStreamInternal::Clone();
}

void AudioSourceInternal::OnDrain() {
  _ondrain();
}

std::shared_ptr<AudioSource> AudioSource::New() {
  return std::make_shared<AudioSourceInternal>();
}

AudioSource::AudioSource() {

}

AudioSource::~AudioSource() {
  
}
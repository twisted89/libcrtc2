
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
#include "audiobuffer.h"

using namespace crtc;

AudioBufferInternal::AudioBufferInternal(const std::shared_ptr<ArrayBuffer>& buffer, int channels, int sampleRate, int bitsPerSample, int frames) :
	ArrayBufferInternal(buffer),
	_channels(channels),
	_samplerate(sampleRate),
	_bitspersample(bitsPerSample),
	_frames(frames)
{ }

AudioBufferInternal::~AudioBufferInternal() {

}

size_t AudioBufferInternal::ByteLength() const {
	return ArrayBufferInternal::ByteLength();
}

std::shared_ptr<ArrayBuffer> AudioBufferInternal::Slice(size_t begin, size_t end) const {
	return ArrayBufferInternal::Slice(begin, end);
}

uint8_t* AudioBufferInternal::Data() {
	return ArrayBufferInternal::Data();
}

const uint8_t* AudioBufferInternal::Data() const {
	return ArrayBufferInternal::Data();
}

std::string AudioBufferInternal::ToString() const {
	return ArrayBufferInternal::ToString();
}

int AudioBufferInternal::Channels() const {
	return _channels;
}

int AudioBufferInternal::SampleRate() const {
	return _samplerate;
}

int AudioBufferInternal::BitsPerSample() const {
	return _bitspersample;
}

int AudioBufferInternal::Frames() const {
	return _frames;
}

std::shared_ptr<AudioBuffer> AudioBuffer::New(int channels, int sampleRate, int bitsPerSample, int frames) {
	return std::make_shared<AudioBufferInternal>(ArrayBuffer::New(sampleRate / 100), channels, sampleRate, bitsPerSample, frames);
}

std::shared_ptr<AudioBuffer> AudioBuffer::New(const std::shared_ptr<ArrayBuffer>& buffer, int channels, int sampleRate, int bitsPerSample, int frames) {
	return std::make_shared<AudioBufferInternal>(buffer, channels, sampleRate, bitsPerSample, frames);
}


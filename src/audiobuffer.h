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

#ifndef CRTC_AUDIOBUFFER_H
#define CRTC_AUDIOBUFFER_H

#include "crtc.h"
#include "arraybuffer.h"

namespace crtc {
	class AudioBufferInternal : public AudioBuffer, public ArrayBufferInternal {
		friend class AudioBuffer;

	public:
		explicit AudioBufferInternal(const std::shared_ptr<ArrayBuffer>& buffer, int channels, int sampleRate, int bitsPerSample, int frames);
		virtual ~AudioBufferInternal();

		size_t ByteLength() const override;

		std::shared_ptr<ArrayBuffer> Slice(size_t begin = 0, size_t end = 0) const override;

		uint8_t* Data() override;
		const uint8_t* Data() const override;

		String ToString() const override;

		int Channels() const override;
		int SampleRate() const override;
		int BitsPerSample() const override;
		int Frames() const override;

	protected:
		int _channels;
		int _samplerate;
		int _bitspersample;
		int _frames;
	};
}

#endif
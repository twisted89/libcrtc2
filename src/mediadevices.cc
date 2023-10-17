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
#include "mediadevices.h"
#include <string> 
#include <future>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/video_decoder_factory_template.h>
#include <api/video_codecs/video_decoder_factory_template_open_h264_adapter.h>
#include "pc/test/fake_video_track_source.h"

using namespace crtc;

std::unique_ptr<rtc::Thread> MediaDevicesInternal::network_thread;
std::unique_ptr<rtc::Thread> MediaDevicesInternal::worker_thread;
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> MediaDevicesInternal::media_factory;
rtc::scoped_refptr<webrtc::AudioDeviceModule> MediaDevicesInternal::audio_device;
std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> MediaDevicesInternal::video_device;

void MediaDevicesInternal::Init() {
	network_thread = rtc::Thread::CreateWithSocketServer();
	network_thread->SetName("network", nullptr);

	if (!network_thread->Start()) {

	}

	worker_thread = rtc::Thread::Create();
	worker_thread->SetName("worker", nullptr);

	if (!worker_thread->Start()) {

	}

	audio_device = webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, nullptr);

	if (!audio_device.get()) {
		// TODO(): Handle Error!
	}

	if (!audio_device->Initialized()) {
		audio_device->Init();
	}

	if (!audio_device->PlayoutIsInitialized()) {
		audio_device->InitPlayout();
	}

	if (!audio_device->RecordingIsInitialized()) {
		audio_device->InitRecording();
	}

	video_device.reset(webrtc::VideoCaptureFactory::CreateDeviceInfo(0));

	if (!video_device) {
		// TODO(): Handle Error!
	}

	media_factory = webrtc::CreatePeerConnectionFactory(
		network_thread.get(),
		worker_thread.get(),
		rtc::Thread::Current(),
		std::move(audio_device),
		nullptr, //  rtc::scoped_refptr<AudioEncoderFactory> audio_encoder_factory,
		nullptr, //rtc::scoped_refptr<AudioDecoderFactory> audio_decoder_factory,
		nullptr, //std::unique_ptr<VideoEncoderFactory> video_encoder_factory,
		std::make_unique<webrtc::VideoDecoderFactoryTemplate<webrtc::OpenH264DecoderTemplateAdapter>>(),
		nullptr, //rtc::scoped_refptr<AudioMixer> audio_mixer,
		nullptr, //rtc::scoped_refptr<AudioProcessing> audio_processing,
		nullptr, //std::unique_ptr<AudioFrameProcessor> owned_audio_frame_processor,
		nullptr); //std::unique_ptr<FieldTrialsView> field_trials = nullptr)

	if (!media_factory.get()) {
		// TODO(): Handle Error!
	}
}

std::vector<blink::WebMediaDeviceInfo> MediaDevices::EnumerateDevices() {
	std::vector<blink::WebMediaDeviceInfo> devices;

	for (int index = 0, devs = MediaDevicesInternal::audio_device->PlayoutDevices(); index < devs; index++) {
		char label[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char guid[webrtc::kAdmMaxGuidSize] = { 0 };

		if (!MediaDevicesInternal::audio_device->PlayoutDeviceName(index, label, guid)) {
			blink::WebMediaDeviceInfo dev;

			dev.device_id = std::to_string(index);
			dev.label = std::string(label);
			dev.group_id = std::string(guid);

			// dev.fac = MediaDeviceInfo::kAudioOutput;

			devices.push_back(dev);
		}
	}

	for (int index = 0, devs = MediaDevicesInternal::audio_device->RecordingDevices(); index < devs; index++) {
		char label[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char guid[webrtc::kAdmMaxGuidSize] = { 0 };

		if (!MediaDevicesInternal::audio_device->RecordingDeviceName(index, label, guid)) {
			blink::WebMediaDeviceInfo dev;

			dev.device_id = std::to_string(index);
			dev.label = std::string(label);
			dev.group_id = std::string(guid);
			//dev.kind = MediaDeviceInfo::kAudioInput;

			devices.push_back(dev);
		}
	}

	for (int index = 0, devs = MediaDevicesInternal::video_device->NumberOfDevices(); index < devs; index++) {
		char label[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char guid[webrtc::kAdmMaxDeviceNameSize] = { 0 };

		if (!MediaDevicesInternal::video_device->GetDeviceName(index, label, webrtc::kAdmMaxDeviceNameSize, guid, webrtc::kAdmMaxDeviceNameSize)) {
			blink::WebMediaDeviceInfo dev;

			dev.device_id = std::to_string(index);
			dev.label = std::string(label);
			dev.group_id = std::string(guid);
			//dev.kind = MediaDeviceInfo::kVideoInput;

			devices.push_back(dev);
		}
	}

	return devices;
}
#include <third_party/blink/renderer/modules/mediastream/media_stream_constraints_util.h>

std::shared_ptr<MediaStream> MediaDevices::GetUserMedia(const MediaStreamConstraints& constraints) {
	if (!constraints.enableAudio && constraints.enableVideo) {
		return nullptr;
	}

	rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track;
	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track;

	if (constraints.enableAudio) {
		cricket::AudioOptions options;
		rtc::scoped_refptr<webrtc::AudioSourceInterface> audio_source = MediaDevicesInternal::media_factory->CreateAudioSource(options);

		if (audio_source.get()) {
			audio_track = MediaDevicesInternal::media_factory->CreateAudioTrack("audio", audio_source.get());
		}
	}

	if (constraints.enableVideo) {

		rtc::scoped_refptr<webrtc::FakeVideoTrackSource> video_source =
			webrtc::FakeVideoTrackSource::Create(/*is_screencast=*/false);
		//rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> video_source(MediaDevicesInternal::media_factory->CreateVideoSource(capturer));

		if (video_source.get()) {
			video_track = MediaDevicesInternal::media_factory->CreateVideoTrack("video", video_source.get());
		}
	}

	if (video_track.get() || audio_track.get()) {
		rtc::scoped_refptr<webrtc::MediaStreamInterface> stream = MediaDevicesInternal::media_factory->CreateLocalMediaStream("stream");

		if (stream.get()) {
			if (video_track.get()) {
				stream->AddTrack(video_track);
			}

			if (audio_track.get()) {
				stream->AddTrack(audio_track);
			}

			return MediaStreamInternal::New(stream);
		}
	}

	return nullptr; // TODO(): Reject!
}

std::shared_ptr<MediaStream> MediaDevices::GetUserMedia() {
	MediaStreamConstraints constraints;

	constraints.enableAudio = true;
	constraints.enableVideo = true;

	return MediaDevices::GetUserMedia(constraints);
}
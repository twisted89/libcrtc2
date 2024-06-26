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

#include "rtcpeerconnection.h"
#include "rtcdatachannel.h"
#include "mediastream.h"
#include "customaudiofactory.h"
#include "customvideofactory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template_open_h264_adapter.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/video_encoder_factory_template.h"
#include "api/video_codecs/video_encoder_factory_template_open_h264_adapter.h"
#include "rtc_base/logging.h"
#include "fakeaudiodevice.h"
#ifdef __ANDROID__
#include <unistd.h>
#endif

using namespace crtc;

RTCPeerConnectionInternal::RTCPeerConnectionInternal() {
	webrtc::PeerConnectionInterface::RTCConfiguration cfg(webrtc::PeerConnectionInterface::RTCConfigurationType::kAggressive);

	_settingLocalDesc = _settingRemoteDesc = false;

	_task_queue = webrtc::CreateDefaultTaskQueueFactory();

	_network_thread = rtc::Thread::CreateWithSocketServer();
	_network_thread->SetName("network", nullptr);

	if (!_network_thread->Start()) {
		rtc::webrtc_logging_impl::LogCall();
	}

	_signal_thread = rtc::Thread::CreateWithSocketServer();
	_signal_thread->SetName("signal", nullptr);

	if (!_signal_thread->Start()) {
		rtc::webrtc_logging_impl::LogCall();
	}

	_worker_thread = rtc::Thread::Create();
	_worker_thread->SetName("worker", nullptr);

	if (!_worker_thread->Start()) {
		rtc::webrtc_logging_impl::LogCall();
	}

	//auto audio_device = webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kDummyAudio, _task_queue.get());
	auto audio_device = FakeAudioDeviceModule::Create(); // new webrtc::FakeAudioDeviceModule();

	//if (audio_device && !audio_device->Initialized()) {
	//	audio_device->Init();
	//}

	_factory = webrtc::CreatePeerConnectionFactory(
		_network_thread.get(),
		_worker_thread.get(),
		_signal_thread.get(),
		audio_device,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		rtc::make_ref_counted<CustomAudioFactory>(this),
		std::make_unique<webrtc::VideoEncoderFactoryTemplate<webrtc::OpenH264EncoderTemplateAdapter>>(),
		std::make_unique<CustomVideoFactory>(this),
		nullptr, //rtc::scoped_refptr<AudioMixer> audio_mixer,
		nullptr, //rtc::scoped_refptr<AudioProcessing> audio_processing,
		nullptr, //std::unique_ptr<AudioFrameProcessor> owned_audio_frame_processor,
		nullptr); //std::unique_ptr<FieldTrialsView> field_trials = nullptr)
}

RTCPeerConnectionInternal::~RTCPeerConnectionInternal() {
	if (_socket && _socket->signaling_state() != webrtc::PeerConnectionInterface::kClosed) {
		_socket->Close();
	}

	for (const auto& s : _streams)
	{
		s->ClearObserver();
	}

	//Process any remaining events before we delete
	while (Module::DispatchEvents(false) || _settingRemoteDesc || _settingRemoteDesc) {
#ifdef __ANDROID__
		sleep(1);
#else
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
	}

	_streams.clear();
}

std::shared_ptr<RTCDataChannel> RTCPeerConnectionInternal::CreateDataChannel(const String& label, const RTCDataChannelInit& options) {
	webrtc::DataChannelInit init;

	init.ordered = options.ordered;
	init.maxRetransmitTime = options.maxPacketLifeTime;
	init.maxRetransmits = options.maxRetransmits;
	init.protocol = options.protocol;
	init.negotiated = options.negotiated;
	init.id = options.id;

	if (_socket)
	{
		//rtc::scoped_refptr<webrtc::DataChannelInterface> channel = _socket->CreateDataChannel(label, &init);
		auto error_or_datachannel = _socket->CreateDataChannelOrError(std::string(label), &init);
		if (!error_or_datachannel.ok())
		{
			return nullptr;
		}
		return std::make_shared<RTCDataChannelInternal>(std::move(error_or_datachannel.value()));
	}

	return nullptr;
}

void RTCPeerConnectionInternal::AddIceCandidate(const RTCPeerConnection::RTCIceCandidate& candidate) {
	Promise<>::New([=](
		const Promise<>::FullFilledCallback& resolve,
		const Promise<>::RejectedCallback& reject) {
			webrtc::SdpParseError error;
			webrtc::IceCandidateInterface* ice = webrtc::CreateIceCandidate(std::string(candidate.sdpMid), candidate.sdpMLineIndex, std::string(candidate.candidate), &error);

			if (ice && _socket) {
				if (!_socket->pending_remote_description() && !_socket->current_remote_description()) {
					_pending_candidates.push_back([=]() {
						if (_socket->AddIceCandidate(ice)) {
							return resolve();
						}

						if (error.description.empty()) {
							if (!_socket->pending_remote_description() && !_socket->current_remote_description()) {
								return reject(Error::New("ICE candidates can't be added without any remote session description.", __FILE__, __LINE__));
							}

							return reject(Error::New("Candidate cannot be used.", __FILE__, __LINE__));
						}
						});
				}
				else {
					if (_socket->AddIceCandidate(ice)) {
						return resolve();
					}

					if (error.description.empty()) {
						if (!_socket->pending_remote_description() && !_socket->current_remote_description()) {
							return reject(Error::New("ICE candidates can't be added without any remote session description.", __FILE__, __LINE__));
						}

						return reject(Error::New("Candidate cannot be used.", __FILE__, __LINE__));
					}
				}
			}

			return reject(Error::New(error.description.c_str()));
		})->WaitForResult();
}

void RTCPeerConnectionInternal::AddStream(const std::shared_ptr<MediaStream>& stream) {
	if (_socket)
		_socket->AddStream(reinterpret_cast<webrtc::MediaStreamInterface*>(stream->GetStream()));
}

/*
std::shared_ptr<RTCPeerConnection::RTCRtpSender> RTCPeerConnectionInternal::AddTrack(const std::shared_ptr<MediaStreamTrack> &track, const std::shared_ptr<MediaStream> &stream) {
  // TODO(): Implement this
  return std::shared_ptr<RTCPeerConnection::RTCRtpSender>();
}
*/

void RTCPeerConnectionInternal::CreateAnswer(std::function<void(RTCPeerConnection::RTCSessionDescription*)> callback, const RTCAnswerOptions& options) {
	Promise<RTCPeerConnection::RTCSessionDescription>::New([=](
		const Promise<RTCPeerConnection::RTCSessionDescription>::FullFilledCallback& resolve,
		const Promise<RTCPeerConnection::RTCSessionDescription>::RejectedCallback& reject) {
			rtc::scoped_refptr<CreateOfferAnswerObserver> observer = rtc::make_ref_counted<CreateOfferAnswerObserver>(resolve, reject);
			webrtc::PeerConnectionInterface::RTCOfferAnswerOptions answer_options(
				true, // offer_to_receive_video
				true, // offer_to_receive_audio
				options.voiceActivityDetection, // voice_activity_detection
				false, // ice_restart 
				true  // use_rtp_mux
			);

			if (observer.get() && _socket) {
				_socket->CreateAnswer(observer.get(), answer_options);
			}
			else {
				reject(Error::New("CreateOfferAnswerObserver Failed", __FILE__, __LINE__));
			}
		}
	)
		->Then([=](RTCPeerConnection::RTCSessionDescription desc) mutable {
			callback(&desc);
			}
		);
}

void RTCPeerConnectionInternal::CreateOffer(std::function<void(RTCPeerConnection::RTCSessionDescription*)> callback, const RTCOfferOptions& options) {

	Promise<RTCPeerConnection::RTCSessionDescription>::New([=](
		const Promise<RTCPeerConnection::RTCSessionDescription>::FullFilledCallback& resolve,
		const Promise<RTCPeerConnection::RTCSessionDescription>::RejectedCallback& reject)
		{
			rtc::scoped_refptr<CreateOfferAnswerObserver> observer = rtc::make_ref_counted<CreateOfferAnswerObserver>(resolve, reject);
			webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offer_options(
				1, // offer_to_receive_video
				1, // offer_to_receive_audio
				options.voiceActivityDetection, // voice_activity_detection
				options.iceRestart, // ice_restart 
				true  // use_rtp_mux
			);

			if (observer.get() && _socket) {
				_socket->CreateOffer(observer.get(), offer_options);
			}
			else {
				reject(Error::New("CreateOfferAnswerObserver Failed", __FILE__, __LINE__));
			}
		}
	)
		->Then([=](RTCPeerConnection::RTCSessionDescription desc) mutable {
			callback(&desc);
			}
		);
}

/*
std::shared_ptr<Promise<RTCPeerConnection::RTCCertificate>> RTCPeerConnectionInternal::GenerateCertificate() {
  std::shared_ptr<RTCPeerConnection> self(this);

  return Promise<RTCPeerConnection::RTCCertificate>::New([=](
	const Promise<RTCPeerConnection::RTCCertificate>::FullFilledCallback &resolve,
	const Promise<RTCPeerConnection::RTCCertificate>::RejectedCallback &reject)
  {
	  // TODO(): Implement this

  });
}
*/

MediaStreams RTCPeerConnectionInternal::GetLocalStreams() {
	MediaStreams streams;
	if (_socket)
	{
		rtc::scoped_refptr<webrtc::StreamCollectionInterface> lstreams(_socket->local_streams());
		for (size_t index = 0; index < lstreams->count(); index++) {
			auto stream = MediaStreamInternal::New(lstreams->at(index));

			if (stream) {
				streams.push_back(stream);
			}
		}
	}

	return streams;
}

MediaStreams RTCPeerConnectionInternal::GetRemoteStreams() {
	MediaStreams streams;
	if (_socket)
	{
		rtc::scoped_refptr<webrtc::StreamCollectionInterface> rstreams(_socket->remote_streams());
		for (size_t index = 0; index < rstreams->count(); index++) {
			auto stream = MediaStreamInternal::New(rstreams->at(index));

			if (stream) {
				streams.push_back(stream);
			}
		}
	}

	return streams;
}

void RTCPeerConnectionInternal::RemoveStream(const std::shared_ptr<MediaStream>& stream) {
	if (_socket)
		_socket->RemoveStream(reinterpret_cast<webrtc::MediaStreamInterface*>(stream->GetStream()));
}

/*
void RTCPeerConnectionInternal::RemoveTrack(const std::shared_ptr<RTCPeerConnection::RTCRtpSender> &sender) {
  // TODO(): Implement this
}
*/

bool RTCPeerConnectionInternal::SetConfiguration(const RTCPeerConnection::RTCConfiguration& config) {
	webrtc::PeerConnectionInterface::RTCConfiguration cfg(webrtc::PeerConnectionInterface::RTCConfigurationType::kAggressive);

	auto error = ParseConfiguration(config, &cfg);

	if (!error) {
		webrtc::PeerConnectionDependencies pc_dependencies(this);
		auto error_or_peer_connection = _factory->CreatePeerConnectionOrError(cfg, std::move(pc_dependencies));
		if (error_or_peer_connection.ok())
		{
			_socket = std::move(error_or_peer_connection.value());
			return true;
		}
	}

	return false;
}

void RTCPeerConnectionInternal::SetLocalDescription(std::shared_ptr<const RTCSessionDescription> sdp) {

	if (!_settingLocalDesc)
	{
		_settingLocalDesc = true;
		Promise<>::New([=](
			const Promise<>::FullFilledCallback& resolve,
			const Promise<>::RejectedCallback& reject)
			{
				auto desc = SDP2SDP(sdp.get());

				if (desc && _socket) {
					auto observer = rtc::make_ref_counted<SetLocalDescriptionObserver>(resolve, reject);
					_socket->SetLocalDescription(std::move(desc), observer);
				}
				else {
					reject(Error::New("Failed to create local description from SDP", __FILE__, __LINE__));
				}
			})->Finally([=]() {
				_settingLocalDesc = false;
			});
	}
}

void RTCPeerConnectionInternal::SetRemoteDescription(std::shared_ptr<const RTCSessionDescription> sdp) {
	if (!_settingRemoteDesc)
	{
		_settingRemoteDesc = true;
		Promise<>::New([=](
			const Promise<>::FullFilledCallback& resolve,
			const Promise<>::RejectedCallback& reject)
			{
				if (_socket) {
					Promise<>::New([=](const Promise<>::FullFilledCallback& res, const Promise<>::RejectedCallback& rej) {
						auto desc = SDP2SDP(sdp.get());
						if (desc)
						{
							auto observer = rtc::make_ref_counted<SetRemoteDescriptionObserver>(res, rej);
							_socket->SetRemoteDescription(std::move(desc), observer);
						}
						else {
							reject(Error::New("Failed to create remote description from SDP", __FILE__, __LINE__));
						}
						})->Then([=]()
							{
								if (_pending_candidates.size()) {
									for (const auto& callback : _pending_candidates) {
										callback();
									}

									_pending_candidates.clear();
								}
								resolve();
							}
						)->Catch([=](const std::shared_ptr<Error>& error)
							{
								reject(error);
							});
				}
				else {
					reject(Error::New("SOCKET is NULL!", __FILE__, __LINE__));
				}
			})->Finally([=]() {
				_settingRemoteDesc = false;
				});
	}
}

void RTCPeerConnectionInternal::Close() {
	if (_socket && _socket->signaling_state() != webrtc::PeerConnectionInterface::kClosed) {
		_socket->Close();
	}
}


RTCPeerConnection::RTCSessionDescription RTCPeerConnectionInternal::CurrentLocalDescription() {
	RTCPeerConnection::RTCSessionDescription sdp;
	if (_socket)
		SDP2SDP(_socket->current_local_description(), &sdp);
	return sdp;
}

RTCPeerConnection::RTCSessionDescription RTCPeerConnectionInternal::CurrentRemoteDescription() {
	RTCPeerConnection::RTCSessionDescription sdp;
	if (_socket)
		SDP2SDP(_socket->current_remote_description(), &sdp);
	return sdp;
}

RTCPeerConnection::RTCSessionDescription RTCPeerConnectionInternal::LocalDescription() {
	RTCPeerConnection::RTCSessionDescription sdp;
	if (_socket)
		SDP2SDP(_socket->local_description(), &sdp);
	return sdp;
}

RTCPeerConnection::RTCSessionDescription RTCPeerConnectionInternal::PendingLocalDescription() {
	RTCPeerConnection::RTCSessionDescription sdp;
	if (_socket)
		SDP2SDP(_socket->pending_local_description(), &sdp);
	return sdp;
}

RTCPeerConnection::RTCSessionDescription RTCPeerConnectionInternal::PendingRemoteDescription() {
	RTCPeerConnection::RTCSessionDescription sdp;
	if (_socket)
		SDP2SDP(_socket->pending_remote_description(), &sdp);
	return sdp;
}

RTCPeerConnection::RTCSessionDescription RTCPeerConnectionInternal::RemoteDescription() {
	RTCPeerConnection::RTCSessionDescription sdp;
	if (_socket)
		SDP2SDP(_socket->remote_description(), &sdp);
	return sdp;
}

RTCPeerConnection::RTCIceConnectionState RTCPeerConnectionInternal::IceConnectionState() {
	if (_socket)
	{
		switch (_socket->ice_connection_state()) {
		case webrtc::PeerConnectionInterface::kIceConnectionNew:
			return RTCPeerConnection::kNew;
		case webrtc::PeerConnectionInterface::kIceConnectionChecking:
			return RTCPeerConnection::kChecking;
		case webrtc::PeerConnectionInterface::kIceConnectionConnected:
		case webrtc::PeerConnectionInterface::kIceConnectionMax:
			return RTCPeerConnection::kConnected;
		case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
			return RTCPeerConnection::kCompleted;
		case webrtc::PeerConnectionInterface::kIceConnectionFailed:
			return RTCPeerConnection::kFailed;
		case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
			return RTCPeerConnection::kDisconnected;
		case webrtc::PeerConnectionInterface::kIceConnectionClosed:
			return RTCPeerConnection::kClosed;
		}
	}

	return RTCPeerConnection::kNew;
}

RTCPeerConnection::RTCIceGatheringState RTCPeerConnectionInternal::IceGatheringState() {
	if (_socket)
	{
		switch (_socket->ice_gathering_state()) {
		case webrtc::PeerConnectionInterface::kIceGatheringNew:
			return RTCPeerConnection::kNewGathering;
		case webrtc::PeerConnectionInterface::kIceGatheringGathering:
			return RTCPeerConnection::kGathering;
		case webrtc::PeerConnectionInterface::kIceGatheringComplete:
			return RTCPeerConnection::kComplete;
		}
	}

	return RTCPeerConnection::kNewGathering;
}

RTCPeerConnection::RTCSignalingState RTCPeerConnectionInternal::SignalingState() {
	if (_socket)
	{
		switch (_socket->signaling_state()) {
		case webrtc::PeerConnectionInterface::kStable:
			return RTCPeerConnection::kStable;
		case webrtc::PeerConnectionInterface::kHaveLocalOffer:
			return RTCPeerConnection::kHaveLocalOffer;
		case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
			return RTCPeerConnection::kHaveLocalPrAnswer;
		case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
			return RTCPeerConnection::kHaveRemoteOffer;
		case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
			return RTCPeerConnection::kHaveRemotePrAnswer;
		case webrtc::PeerConnectionInterface::kClosed:
			return RTCPeerConnection::kSignalingClosed;
		}
	}

	return RTCPeerConnection::kStable;
}

void RTCPeerConnectionInternal::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
	_onsignalingstatechange();

	if (new_state == webrtc::PeerConnectionInterface::kClosed) {
		_event.reset();
	}
	else if (!_event) {
		_event = Event::New();
	}
}

void RTCPeerConnectionInternal::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
	_streams.emplace_back(MediaStreamInternal::New(stream.get()));
	_streams.back()->onAddTrack(std::bind(&RTCPeerConnectionInternal::OnMediaTrack, this, std::placeholders::_1));
	_streams.back()->onRemoveTrack(std::bind(&RTCPeerConnectionInternal::OnRemoveMediaTrack, this, std::placeholders::_1));
	_onaddstream(_streams.back());
}

void RTCPeerConnectionInternal::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
	for (const auto& s : _streams)
	{
		if (s->IdString() == stream->id()) {
			_onremovestream(s);
			//_streams.erase(it);
			break;
		}
	}
}

void RTCPeerConnectionInternal::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
	//Ignore these as we're unable to control their lifecycle, use tracks from mediastream instead!
	(void)transceiver;
}

void crtc::RTCPeerConnectionInternal::OnMediaTrack(std::shared_ptr<MediaStreamTrack> track)
{
	_onaddtrack(track);
}

void RTCPeerConnectionInternal::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
	//Ignore these as we're unable to control their lifecycle, use tracks from mediastream instead!
	(void)receiver;
}

void crtc::RTCPeerConnectionInternal::OnRemoveMediaTrack(std::shared_ptr<MediaStreamTrack> track)
{
	_onremovetrack(track);
}

void RTCPeerConnectionInternal::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
	if (data_channel.get()) {
		auto channel = std::make_shared<RTCDataChannelInternal>(data_channel);

		if (channel) {
			_ondatachannel(channel);
		}
	}
}

void RTCPeerConnectionInternal::OnRenegotiationNeeded() {
	_onnegotiationneeded();
}

void RTCPeerConnectionInternal::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
	_oniceconnectionstatechange();
}

void RTCPeerConnectionInternal::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
	_onicegatheringstatechange();
}

void RTCPeerConnectionInternal::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
	auto iceCandidate = std::make_shared<RTCPeerConnection::RTCIceCandidate>();

	iceCandidate->sdpMid = candidate->sdp_mid().c_str();
	iceCandidate->sdpMLineIndex = candidate->sdp_mline_index();

	std::string candidateStr;

	if (candidate->ToString(&candidateStr)) {
		iceCandidate->candidate = candidateStr.c_str();
		_onicecandidate(iceCandidate);
	}
}

void crtc::RTCPeerConnectionInternal::OnIceCandidateError(const std::string& address, int port, const std::string& url, int error_code, const std::string& error_text)
{

}

void RTCPeerConnectionInternal::OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) {
	_onicecandidatesremoved();
}

void RTCPeerConnectionInternal::OnIceConnectionReceivingChange(bool receiving) {
	//oniceconnectionstatechange();
}

bool crtc::RTCPeerConnectionInternal::BypassVideoDecoder()
{
	return _onRawVideo;
}

bool crtc::RTCPeerConnectionInternal::BypassAudioDecoder()
{
	return _onRawAudio;
}

void crtc::RTCPeerConnectionInternal::onRawVideo(const webrtc::EncodedImage& input_image, int64_t render_time_ms)
{
	if (_onRawVideo)
		_onRawVideo(input_image.data(), input_image.size(), input_image.FrameType() == webrtc::VideoFrameType::kVideoFrameKey, render_time_ms);
}

void crtc::RTCPeerConnectionInternal::onRawAudio(const uint8_t* data, size_t data_length)
{
	if (_onRawAudio)
		_onRawAudio(data, data_length);
}

void crtc::RTCPeerConnectionInternal::onRawVideo(std::function<void((const unsigned char* data, size_t length, bool isKeyFrame, int64_t renderTimeMs))> callback)
{
	_onRawVideo = callback;
}

void crtc::RTCPeerConnectionInternal::onRawAudio(std::function<void((const unsigned char* data, size_t length))> callback)
{
	_onRawAudio = callback;
}

void crtc::RTCPeerConnectionInternal::onAddTrack(std::function<void(const std::shared_ptr<MediaStreamTrack>)> callback)
{
	_onaddtrack = callback;
}

void crtc::RTCPeerConnectionInternal::onRemoveTrack(std::function<void(const std::shared_ptr<MediaStreamTrack>)> callback)
{
	_onremovetrack = callback;
}

void crtc::RTCPeerConnectionInternal::onAddStream(std::function<void(const std::shared_ptr<MediaStream>)> callback)
{
	_onaddstream = callback;
}

void crtc::RTCPeerConnectionInternal::onRemoveStream(std::function<void(const std::shared_ptr<MediaStream>)> callback)
{
	_onremovestream = callback;
}

void crtc::RTCPeerConnectionInternal::onDataChannel(std::function<void(const std::shared_ptr<RTCDataChannel>)> callback)
{
	_ondatachannel = callback;
}

void crtc::RTCPeerConnectionInternal::onIceCandidate(std::function<void(const std::shared_ptr<RTCIceCandidate>)> callback)
{
	_onicecandidate = callback;
}

void crtc::RTCPeerConnectionInternal::onNegotiationNeeded(std::function<void()> callback)
{
	_onnegotiationneeded = callback;
}

void crtc::RTCPeerConnectionInternal::onsignalingstatechange(std::function<void()> callback)
{
	_onsignalingstatechange = callback;
}

void crtc::RTCPeerConnectionInternal::onIceGatheringStateChange(std::function<void()> callback)
{
	_onicegatheringstatechange = callback;
}

void crtc::RTCPeerConnectionInternal::onIceConnectionStateChange(std::function<void()> callback)
{
	_oniceconnectionstatechange = callback;
}

void crtc::RTCPeerConnectionInternal::onIceCandidatesRemoved(std::function<void()> callback)
{
	_onicecandidatesremoved = callback;
}

// DEPRECATED -> //
/*
void RTCPeerConnectionInternal::OnAddStream(webrtc::MediaStreamInterface* stream) {

}

void RTCPeerConnectionInternal::OnDataChannel(webrtc::DataChannelInterface* data_channel) {

}

void RTCPeerConnectionInternal::OnRemoveStream(webrtc::MediaStreamInterface* stream) {

}
*/
// <- DEPRECATED //


std::shared_ptr<RTCPeerConnection> RTCPeerConnection::New(const RTCPeerConnection::RTCConfiguration& config) {
	auto pc = std::make_shared<RTCPeerConnectionInternal>();
	if (pc && pc->SetConfiguration(config))
		return pc;
	return nullptr;
}

RTCPeerConnection::RTCConfiguration::RTCConfiguration() :
	iceCandidatePoolSize(0),
	bundlePolicy(kMaxBundle),
	iceTransportPolicy(kAll),
	rtcpMuxPolicy(kRequire)
{
	RTCIceServer iceserver;
	iceserver.urls.push_back(String("stun:stun.l.google.com:19302"));
	iceServers.push_back(iceserver);
}

RTCPeerConnection::RTCConfiguration::~RTCConfiguration() {

}

RTCPeerConnection::RTCPeerConnection() {

}

RTCPeerConnection::~RTCPeerConnection() {

}

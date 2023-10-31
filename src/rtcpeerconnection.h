
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

#ifndef CRTC_RTCPEERCONNECTION_H
#define CRTC_RTCPEERCONNECTION_H

#include "crtc.h"
#include "event.h"
#include "utils.hpp"
#include "promise.h"

#include <api/peer_connection_interface.h>
#include <api/create_peerconnection_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <media/engine/webrtc_video_engine.h>
#include <modules/audio_device/include/audio_device.h>
#include <modules/video_coding/codecs/h264/include/h264.h>

namespace crtc {
	class RTCPeerConnectionInternal;

	class RTCPeerConnectionInternal : public RTCPeerConnection, public webrtc::PeerConnectionObserver {
		friend class RTCPeerConnectionObserver;

	public:
		explicit RTCPeerConnectionInternal();
		virtual ~RTCPeerConnectionInternal() override;

		std::shared_ptr<RTCDataChannel> CreateDataChannel(const String& label, const RTCDataChannelInit& options = RTCDataChannelInit()) override;
		void AddIceCandidate(const RTCPeerConnection::RTCIceCandidate& candidate) override;
		void AddStream(const std::shared_ptr<MediaStream>& stream) override;
		// Let<RTCPeerConnection::RTCRtpSender> AddTrack(const Let<MediaStreamTrack> &track, const Let<MediaStream> &stream) override;
		void CreateAnswer(std::function<void(RTCPeerConnection::RTCSessionDescription*)> callback, const RTCAnswerOptions& options) override;
		void CreateOffer(std::function<void(RTCPeerConnection::RTCSessionDescription*)> callback, const RTCOfferOptions& options) override;
		// Let<Promise<RTCPeerConnection::RTCCertificate>> GenerateCertificate() override;
		MediaStreams GetLocalStreams() override;
		MediaStreams GetRemoteStreams() override;
		void RemoveStream(const std::shared_ptr<MediaStream>& stream) override;
		// void RemoveTrack(const Let<RTCPeerConnection::RTCRtpSender> &sender) override;
		void SetLocalDescription(std::shared_ptr<const RTCSessionDescription> sdp) override;
		void SetRemoteDescription(std::shared_ptr<const RTCSessionDescription> sdp) override;
		void Close() override;

		bool SetConfiguration(const RTCPeerConnection::RTCConfiguration& config);
		RTCPeerConnection::RTCSessionDescription CurrentLocalDescription() override;
		RTCPeerConnection::RTCSessionDescription CurrentRemoteDescription() override;
		RTCPeerConnection::RTCSessionDescription LocalDescription() override;
		RTCPeerConnection::RTCSessionDescription PendingLocalDescription() override;
		RTCPeerConnection::RTCSessionDescription PendingRemoteDescription() override;
		RTCPeerConnection::RTCSessionDescription RemoteDescription() override;
		RTCPeerConnection::RTCIceConnectionState IceConnectionState() override;
		RTCPeerConnection::RTCIceGatheringState IceGatheringState() override;
		RTCPeerConnection::RTCSignalingState SignalingState() override;

		bool BypassDecoder() override;
		void onRawVideo(const webrtc::EncodedImage& input_image, int64_t render_time_ms);

		void onRawVideo(std::function<void(const unsigned char* data, size_t length, bool isKeyFrame, int64_t renderTimeMs)> callback) override;
		void onAddTrack(std::function<void(const std::shared_ptr<MediaStreamTrack>)> callback) override;
		void onRemoveTrack(std::function<void(const std::shared_ptr<MediaStreamTrack>)> callback) override;
		void onAddStream(std::function<void(const std::shared_ptr<MediaStream>)> callback) override;
		void onRemoveStream(std::function<void(const std::shared_ptr<MediaStream>)> callback) override;
		void onDataChannel(std::function<void(const std::shared_ptr<RTCDataChannel>)> callback) override;
		void onIceCandidate(std::function<void(const std::shared_ptr<RTCIceCandidate>)> callback) override;
		void onNegotiationNeeded(std::function<void()> callback) override;
		void onsignalingstatechange(std::function<void()> callback) override;
		void onIceGatheringStateChange(std::function<void()> callback) override;
		void onIceConnectionStateChange(std::function<void()> callback) override;
		void onIceCandidatesRemoved(std::function<void()> callback) override;

	private:
		inline static std::shared_ptr<Error> SDP2SDP(const webrtc::SessionDescriptionInterface* desc = nullptr, RTCPeerConnection::RTCSessionDescription* sdp = nullptr) {
			if (desc && sdp) {
				if (desc->type().compare(webrtc::SessionDescriptionInterface::kOffer) == 0) {
					sdp->type = RTCPeerConnection::RTCSessionDescription::kOffer;
				}
				else if (desc->type().compare(webrtc::SessionDescriptionInterface::kAnswer) == 0) {
					sdp->type = RTCPeerConnection::RTCSessionDescription::kAnswer;
				}
				else {
					sdp->type = RTCPeerConnection::RTCSessionDescription::kPranswer;
				}

				std::string sdpStr;

				if (desc->ToString(&sdpStr)) {
					sdp->sdp = sdpStr.c_str();
					return nullptr;
				}

				return Error::New("Unable to create SessionDescription", __FILE__, __LINE__);
			}

			return Error::New("Invalid SessionDescriptionInterface", __FILE__, __LINE__);
		}

		inline static std::shared_ptr<Error> SDP2SDP(const RTCPeerConnection::RTCSessionDescription* sdp, webrtc::SessionDescriptionInterface** desc = nullptr) {
			std::string type;
			webrtc::SdpParseError error;

			if (desc) {
				switch (sdp->type) {
				case RTCPeerConnection::RTCSessionDescription::kAnswer:
					type = webrtc::SessionDescriptionInterface::kAnswer;
					break;
				case RTCPeerConnection::RTCSessionDescription::kOffer:
					type = webrtc::SessionDescriptionInterface::kOffer;
					break;
				case RTCPeerConnection::RTCSessionDescription::kPranswer:
					type = webrtc::SessionDescriptionInterface::kPrAnswer;
					break;
				case RTCPeerConnection::RTCSessionDescription::kRollback:
					break;
				}

				*desc = webrtc::CreateSessionDescription(type, std::string(sdp->sdp), &error);

				if (*desc) {
					return nullptr;
				}
				else {
					return Error::New(error.description.c_str(), __FILE__, __LINE__);
				}
			}
			else {
				return Error::New("Invalid SessionDescriptionInterface", __FILE__, __LINE__);
			}
		}

		inline static std::shared_ptr<Error> ParseConfiguration(
			const RTCPeerConnection::RTCConfiguration& config,
			webrtc::PeerConnectionInterface::RTCConfiguration* cfg = nullptr)
		{
			if (cfg) {
				// cfg->certificates = config.certificates;

				cfg->ice_candidate_pool_size = config.iceCandidatePoolSize;

				switch (config.iceTransportPolicy) {
				case RTCPeerConnection::kRelay:
					cfg->type = webrtc::PeerConnectionInterface::kRelay;
					break;
				case RTCPeerConnection::kPublic:
					cfg->type = webrtc::PeerConnectionInterface::kNoHost;
					break;
				case RTCPeerConnection::kAll:
					cfg->type = webrtc::PeerConnectionInterface::kAll;
					break;
				}

				switch (config.rtcpMuxPolicy) {
				case RTCPeerConnection::kNegotiate:
					cfg->rtcp_mux_policy = webrtc::PeerConnectionInterface::kRtcpMuxPolicyNegotiate;
					break;
				case RTCPeerConnection::kRequire:
					cfg->rtcp_mux_policy = webrtc::PeerConnectionInterface::kRtcpMuxPolicyRequire;
					break;
				}

				switch (config.bundlePolicy) {
				case RTCPeerConnection::kBalanced:
					cfg->bundle_policy = webrtc::PeerConnectionInterface::kBundlePolicyBalanced;
					break;
				case RTCPeerConnection::kMaxBundle:
					cfg->bundle_policy = webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle;
					break;
				case RTCPeerConnection::kMaxCompat:
					cfg->bundle_policy = webrtc::PeerConnectionInterface::kBundlePolicyMaxCompat;
					break;
				}

				for (const auto& iceserver : config.iceServers) {
					webrtc::PeerConnectionInterface::IceServer server;

					for (auto& url : iceserver.urls)
					{
						server.urls.emplace_back(url);
					}
					server.username = iceserver.username;
					server.password = iceserver.credential;

					cfg->servers.push_back(server);
				}

				return nullptr;
			}

			return Error::New("Invalid RTCConfiguration", __FILE__, __LINE__);
		}

		class CreateOfferAnswerObserver : public webrtc::CreateSessionDescriptionObserver {
		public:
			CreateOfferAnswerObserver(const Promise<RTCPeerConnection::RTCSessionDescription>::FullFilledCallback& resolve,
				const Promise<RTCPeerConnection::RTCSessionDescription>::RejectedCallback& reject) :
				_resolve(resolve),
				_reject(reject)
			{ }

			~CreateOfferAnswerObserver() override { }

		private:
			void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
				RTCPeerConnection::RTCSessionDescription sdp;

				auto error = SDP2SDP(desc, &sdp);

				if (!error) {
					_resolve(sdp);
				}
				else {
					_reject(error);
				}
			}

			void OnFailure(webrtc::RTCError error) override {
				_reject(Error::New(error.message(), __FILE__, __LINE__));
			}

			Promise<RTCPeerConnection::RTCSessionDescription>::FullFilledCallback _resolve;
			Promise<RTCPeerConnection::RTCSessionDescription>::RejectedCallback _reject;
		};

		class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
		public:
			SetSessionDescriptionObserver(const Promise<>::FullFilledCallback& resolve,
				const Promise<>::RejectedCallback& reject) :
				_resolve(resolve),
				_reject(reject)
			{ }

			~SetSessionDescriptionObserver() override { }

		private:
			void OnSuccess() override {
				_resolve();
			}

			void OnFailure(webrtc::RTCError error) override {
				_reject(Error::New(error.message(), __FILE__, __LINE__));
			}

			Promise<>::FullFilledCallback _resolve;
			Promise<>::RejectedCallback _reject;
		};

		std::unique_ptr<rtc::Thread> _network_thread;
		std::unique_ptr<rtc::Thread> _worker_thread;
		std::unique_ptr<rtc::Thread> _signal_thread;
		std::unique_ptr<webrtc::TaskQueueFactory> _task_queue;
		rtc::scoped_refptr<webrtc::AudioDeviceModule> _audio_device;
		rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _factory;

	protected:
		void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
		void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
		void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
		void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
		void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
		void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
		void OnRenegotiationNeeded() override;
		void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
		void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
		void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
		void OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) override;
		void OnIceConnectionReceivingChange(bool receiving) override;

		rtc::scoped_refptr<webrtc::PeerConnectionInterface> _socket;
		std::shared_ptr<Event> _event;
		std::vector<std::function<void()>> _pending_candidates;

		synchronized_callback<> _onnegotiationneeded;
		synchronized_callback<> _onsignalingstatechange;
		synchronized_callback<> _onicegatheringstatechange;
		synchronized_callback<> _oniceconnectionstatechange;
		synchronized_callback<> _onicecandidatesremoved;
		synchronized_callback<const std::shared_ptr<MediaStream>> _onaddstream;
		synchronized_callback<const std::shared_ptr<MediaStream>> _onremovestream;
		synchronized_callback<const unsigned char*, size_t, bool, int64_t> _onRawVideo;
		synchronized_callback<const std::shared_ptr<MediaStreamTrack>> _onaddtrack;
		synchronized_callback<const std::shared_ptr<MediaStreamTrack>> _onremovetrack;
		synchronized_callback<const std::shared_ptr<RTCDataChannel>> _ondatachannel;
		synchronized_callback<const std::shared_ptr<RTCIceCandidate>> _onicecandidate;


};
}

#endif
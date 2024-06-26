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
#include "module.h"
#include "rtcpeerconnection.h"
#include "rtc_base/thread.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/physical_socket_server.h"
#include <base/atomicops.h>

#if defined(_MSC_VER)
    #include "rtc_base/win32_socket_init.h"
#endif

using namespace crtc;

volatile intptr_t ModuleInternal::pending_events = 0;
synchronized_callback<> asyncCallback;

class Thread : public rtc::AutoThread {
public:
    Thread() : rtc::AutoThread() {
    }

    ~Thread() {
        rtc::Thread::Stop();
    }

    virtual void PostTaskImpl(absl::AnyInvocable<void()&&> task,
        const PostTaskTraits& traits,
        const webrtc::Location& location) override
    {
        asyncCallback();
        rtc::Thread::PostTaskImpl(std::move(task), traits, location);
    }

    virtual void PostDelayedTaskImpl(absl::AnyInvocable<void()&&> task,
        webrtc::TimeDelta delay,
        const PostDelayedTaskTraits& traits,
        const webrtc::Location& location) override
    {
        asyncCallback();
        rtc::Thread::PostDelayedTaskImpl(std::move(task), delay, traits, location);
    }
};

Thread currentThread;

void Module::Init() {
    rtc::ThreadManager::Instance()->SetCurrentThread(&currentThread);
//#ifdef NDEBUG
//    rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
//#else
	rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
//#endif
	rtc::InitializeSSL();
}

void Module::Dispose() {
	rtc::CleanupSSL();
}

bool Module::DispatchEvents(bool kForever) {
	bool result = false;
	//rtc::Thread* thread = rtc::ThreadManager::Instance()->CurrentThread();

	do {
		result = (base::subtle::NoBarrier_Load(&ModuleInternal::pending_events) > 0 && currentThread.ProcessMessages(kForever ? 1000 : 0));
	} while (kForever && result);

	return result;
}

void Module::RegisterAsyncCallback(const std::function<void()>& callback) {
    asyncCallback = callback;
}

void Module::UnregisterAsyncCallback() {
    asyncCallback = nullptr;
}

void Async::Call(std::function<void()> callback, int delayMs) {
    //rtc::Thread* target = rtc::ThreadManager::Instance()->CurrentThread();
    auto event = Event::New();
    if (delayMs > 0) {
        currentThread.PostDelayedTask([callback, event]() { callback(); }, webrtc::TimeDelta::Millis(delayMs));
    }
    else {
        currentThread.PostTask([callback, event]() { callback(); });
    }
}

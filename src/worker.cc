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
#include "worker.h"
#include "rtc_base/time_utils.h"

using namespace crtc;

thread_local WorkerInternal* WorkerInternal::current_worker;

void WorkerInternal::Run() {
  WorkerInternal::current_worker = this;
  ProcessMessages(rtc::ThreadManager::kForever);
  WorkerInternal::current_worker = nullptr;
}

WorkerInternal::WorkerInternal() : rtc::NullSocketServer(), rtc::Thread(this) {
  SetName("worker", nullptr);
}

void WorkerInternal::Call(Callback callback, int delayMs) {
    if (delayMs > 0) {
        PostDelayedTask([callback]() { callback(); }, webrtc::TimeDelta::Millis(delayMs));
    }
    else {
        PostTask([callback]() { callback(); });
    }
}

WorkerInternal::~WorkerInternal() {
  rtc::Thread::Stop();
}

std::shared_ptr<Worker> Worker::New(const Callback &runnable) {
  std::shared_ptr<WorkerInternal> worker = std::make_shared<WorkerInternal>();
  
  if (worker) {
    if (worker->Start()) {
      if (!runnable.IsEmpty()) {
          worker->Call(runnable, 0);
      }

      return worker;
    }
  }

  return nullptr;
}

Worker* Worker::This() {
  return WorkerInternal::current_worker;
}
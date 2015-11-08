// Copyright 2010-2015, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "base/init.h"

#include <vector>

#include "base/logging.h"
#include "base/mutex.h"
#include "base/port.h"
#include "base/singleton.h"

namespace mozc {
namespace {

class Initializer {
 public:
  void Call() {
    scoped_lock l(&mutex_);
    VLOG(1) << "Initializer is called";
    for (size_t i = 0; i < funcs_.size(); ++i) {
      funcs_[i]();
    }
    // clear func not to be called twice
    funcs_.clear();
    funcs_.shrink_to_fit();
  }

  void Add(RegisterModuleFunction func) {
    scoped_lock l(&mutex_);
    funcs_.push_back(func);
  }

 private:
  Mutex mutex_;
  vector<RegisterModuleFunction> funcs_;
};

class Reloader {
 public:
  void Call() {
    scoped_lock l(&mutex_);
    for (size_t i = 0; i < funcs_.size(); ++i) {
      funcs_[i]();
    }
  }

  void Add(RegisterModuleFunction func) {
    scoped_lock l(&mutex_);
    funcs_.push_back(func);
  }

 private:
  Mutex mutex_;
  vector<RegisterModuleFunction> funcs_;
};

class ShutdownHandler {
 public:
  void Call() {
    scoped_lock l(&mutex_);
    VLOG(1) << "ShutdownHandler is called";
    // Call functions in reverse order, as later registered modules may depend
    // on earlier modules.
    for (auto iter = funcs_.rbegin(); iter != funcs_.rend(); ++iter) {
      (*iter)();
    }
    // clear func not to be called twice
    funcs_.clear();
    funcs_.shrink_to_fit();
  }

  void Add(RegisterModuleFunction func) {
    scoped_lock l(&mutex_);
    funcs_.push_back(func);
  }

 private:
  Mutex mutex_;
  vector<RegisterModuleFunction> funcs_;
};

}  // namespace
}  // namespace mozc

namespace mozc {

InitializerRegister::InitializerRegister(const char *name,
                                         RegisterModuleFunction func) {
  Singleton<Initializer>::get()->Add(func);
}

void RunInitializers() {
  Singleton<Initializer>::get()->Call();
}

}   // namespace mozc

namespace mozc {

ReloaderRegister::ReloaderRegister(const char *name,
                                   RegisterModuleFunction func) {
  Singleton<Reloader>::get()->Add(func);
}

void RunReloaders() {
  Singleton<Reloader>::get()->Call();
}

ShutdownHandlerRegister::ShutdownHandlerRegister(const char *name,
                                                 RegisterModuleFunction func) {
  Singleton<ShutdownHandler>::get()->Add(func);
}

void RunShutdownHandlers() {
  Singleton<ShutdownHandler>::get()->Call();
}

}  // namespace mozc

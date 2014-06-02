// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GSSOUI_GSSOUI_EXTENSION_H_
#define GSSOUI_GSSOUI_EXTENSION_H_

#ifndef TIZEN
#include <glib.h>
#include <thread>
#endif  // TIZEN

#include "common/extension.h"

class GssouiExtension : public common::Extension {
 public:
  GssouiExtension();
  virtual ~GssouiExtension();

 private:
  // common::Extension implementation
  virtual common::Instance* CreateInstance();
#ifdef TIZEN
  void SetupMainLoop(void* data);
  GMainLoop* main_loop_;
  static std::thread *t_;
#endif  // TIZEN

};

#endif  // GSSOUI_GSSOUI_EXTENSION_H_

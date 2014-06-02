// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gssoui/gssoui_extension.h"

#include "gssoui/gssoui_instance.h"
#include "gssoui/gssoui_logs.h"

#ifndef NDEBUG
std::ofstream _gsso_f_log;
#endif  // NDEBUG

common::Extension* CreateExtension() {
  return new GssouiExtension();
}

// This will be generated from gssoui_api.js
extern const char kSource_gssoui_api[];

#ifdef TIZEN
// static
void GssouiExtension::SetupMainloop(void* data) {
  GssouiExtension* self = reinterpret_cast<GssouiExtension*>(data);
  g_main_loop_run(self->main_loop_);
}
#endif  // TIZEN

GssouiExtension::GssouiExtension()
#ifdef TIZEN
    : main_loop_(g_main_loop_new(0, FALSE))
    , t_(SpeechInstance::SetupMainloop, this) {
  t_.detach();
#else
{
#endif // TIZEN
  LOG_INIT();
  DBG("Extension created");
  SetExtensionName("tizen.gssoui");
  SetJavaScriptAPI(kSource_gssoui_api);
}

GssouiExtension::~GssouiExtension() {
  DBG("Extension dead");
  GssouiInstance::Shutdown();
#ifdef TIZEN
  g_main_loop_quit(main_loop_);
  g_main_loop_unref(main_loop_);
#endif  // TIZEN
  LOG_CLOSE();
}

common::Instance* GssouiExtension::CreateInstance() {
  DBG("Init instance");
  return new GssouiInstance;
}

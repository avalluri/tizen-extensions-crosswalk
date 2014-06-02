// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GSSOUI_GSSOUI_INSTANCE_H_
#define GSSOUI_GSSOUI_INSTANCE_H_

#include <glib.h>
#include <memory>

#include "common/extension.h"
#include "common/picojson.h"
#include "common/utils.h"

class GssouiServer;

class GssouiInstance : public common::Instance {
 public:
  GssouiInstance();
  virtual ~GssouiInstance();

  static void Shutdown();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* messag){ }
  virtual void HandleSyncMessage(const char* message) { }

  void PostMessage(const picojson::value &msg);
  void PostMessage(const picojson::value::object &msg);

  static gboolean ShutdownServerTimeoutCb(gpointer data);

  static uint32_t instance_counter_;
  static guint timeout_id_;
  static std::shared_ptr<GssouiServer> server_;
};

#endif  // GSSOUI_GSSOUI_INSTANCE_H_

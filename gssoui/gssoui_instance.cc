// Copyright(c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gssoui/gssoui_instance.h"

#include "gssoui/gssoui_logs.h"
#include "gssoui/gssoui_server.h"

const guint kServerTimeout = 5000; // 5 second

uint32_t GssouiInstance::instance_counter_ = 0;
guint GssouiInstance::timeout_id_ = 0;
std::shared_ptr<GssouiServer> GssouiInstance::server_(NULL);

GssouiInstance::GssouiInstance() {
  if (timeout_id_)
    g_source_remove(timeout_id_);

  if ( ++instance_counter_ == 1 && !server_.get())
    server_.reset(new GssouiServer());
  server_->SetActiveInstance(
      static_cast<common::Instance*>(this));
}

GssouiInstance::~GssouiInstance() {
  DBG("Instance dead...");
  server_->SetActiveInstance(NULL);

  if ( --instance_counter_ == 0)
    timeout_id_ = g_timeout_add(kServerTimeout,
                                GssouiInstance::ShutdownServerTimeoutCb, NULL);

}

// static
void GssouiInstance::Shutdown() {
  if (timeout_id_) {
    g_source_remove(timeout_id_);
    timeout_id_ = 0;
  }
  
  server_.reset();
}

// staic
gboolean GssouiInstance::ShutdownServerTimeoutCb(gpointer /*data*/) {
  DBG("server timeout reached, killing server...");
  timeout_id_ = 0;
  server_.reset();
  return FALSE;
}

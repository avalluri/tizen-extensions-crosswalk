// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ping/ping_extension.h"

#include "ping/ping_instance.h"

extern const char kSource_ping_api[];

common::Extension* CreateExtension() {
  return new PingExtension;
}

PingExtension::PingExtension() {
  SetExtensionName("tizen.ping");
  SetJavaScriptAPI(kSource_ping_api);
}

common::Instance* PingExtension::CreateInstance() {
  return new PingInstance;
}

PingExtension::~PingExtension() {}

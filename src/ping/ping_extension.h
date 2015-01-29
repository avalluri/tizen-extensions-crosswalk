// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_PING_EXTENSION_H_
#define TIZEN_PING_EXTENSION_H_

#include "common/extension.h"

class PingExtension : public common::Extension {
 public:
  PingExtension();
  virtual ~PingExtension();
  virtual common::Instance* CreateInstance();
};

#endif  // TIZEN_PING_EXTENSION_H_

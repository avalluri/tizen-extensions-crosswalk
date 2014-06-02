// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GSSOUI_GSSOUI_DATA_H_
#define GSSOUI_GSSOUI_DATA_H_

#include <glib.h>

#include "common/utils.h"

const gchar kUiKeyQueryPassword[] = "QueryPassword";  // bool
const gchar kUiKeyQueryUsername[] = "QueryUserName";  // bool
const gchar kUiKeyConfirm[] = "Confirm";  // bool
const gchar kUiKeyOpenUrl[] = "OpenUrl";  // string(url)
const gchar kUiKeyFinalUrl[] = "FinalUrl";  // string)url)
const gchar kUiKeyRequestId[] = "RequestId"; // string
const gchar kUiKeyForgotPassword[] = "ForgotPassword";  // bool
const gchar kUiForgotPasswordUrl[] = "ForgotPasswordUrl";  // string(url)
const gchar kUiKeyTitle[] = "Title";  // string
const gchar kUiKeyCaption[] = "Caption";  // stirng - ???
const gchar kUiKeyUserName[] = "UserName";  // string
const gchar kUiKeySecret[] = "Secret";  // string
const gchar kUiKeyMessage[] = "Message";  // string ???
const gchar kUiKeyRememberPassword[] = "RememberPassword";  // bool
const gchar kUiCaptchaUrl[] = "CaptchaUrl";  // string(url)

// only occur in reply
const gchar kUiKeyErrorCode[] = "QueryErrorCode";  // int32
const gchar kUiKeyUrlResponse[] = "UrlResponse";  // string:url
const gchar kUiKeyCaptchaResponse[] = "CaptchaResponse";  // string

class GssouiData {
 public:
  enum Error {
    kErroNone = 0,
    // Generic error during interaction.
    kGeneral,
    // Cannot send request to signon-ui.
    kNoSignonui,
    // Signon-Ui cannot create dialog based on the given UiSessionData.
    kBadParameters,
    // User canceled action. Plugin should not retry automatically after this.
    kCancled,
    // Requested ui is not available. For example browser cannot be started.
    kNotAvailable,
    // Given url was not valid.
    kBadUrl,
    // Given captcha image was not valid.
    kBadCaptcha,
    // Given url for capctha loading was not valid.
    kBadCaptchaUrl,
    // Refresh failed.
    kRefreshFailed,
    // Showing ui forbidden by ui policy.
    kForbidden,
    // User pressed forgot password.
    kForgotPassword 
  };

  GssouiData(GVariant* data = NULL);
  ~GssouiData();

  GVariant* ToVariant() const;
  bool GetBool(const char* key) const;
  const char* GetString(const char* key) const;
  bool HasKey(const char* key) const;

  void Add(const char* key, GVariant* value);
  void Remove(const char* key);
#ifndef NDEBUG
  void Dump(const char* msg = "Parameters") const;
#endif  // NDEBUG

 private:
  GHashTable* map_;

  DISALLOW_COPY_AND_ASSIGN(GssouiData);
};

#endif  // GSSOUI_GSSOUI_DATA_H_

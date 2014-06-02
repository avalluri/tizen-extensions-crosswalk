// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function DBG(msg) {
  console.log('[ssoui-D]: ' + msg);
}

function WARN(msg) {
  console.warn('[ssoui-W]: ' + msg);
}

function postMessage(msg) {
  extension.postMessage(JSON.stringify(msg));
};

var sendSyncMessage = function(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  var cmd = msg.cmd;
  DBG(json);

  delete msg.cmd;
  
  if (cmd == 'ShowDialog') {
    if (exports.onshowdialog) exports.onshowdialog(msg.params);
  } else if (cmd == 'RefreshDialog') {
    if (exports.onrefreshdialog) exports.onrefreshdialog(msg.paramps);
  } else if (cmd == 'CloseDialog') {
    if (exports.onclosedialog) exports.onclosedialog();
  } else {
    console.warn('[ssoui-W]: Ignorning molformed message: ' + json);
  }
});

function SSOUIObject() {
  DBG("Initializing ....");
  this.onshowdialog;
  this.onrefreshdialog;
  this.onclosedialog;

  this.sendResults = function(params) {
    DBG('Results: ' + params);
    postMessage(params);
  };

  this.onError = function(params) {
  };
}

exports = new SSOUIObject();


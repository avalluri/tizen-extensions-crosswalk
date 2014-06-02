// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GSSOUI_GSSOUI_SERVER_H_
#define GSSOUI_GSSOUI_SERVER_H_

#include <libsoup/soup.h>
#include <gio/gio.h>
#include <glib.h>

#include <queue>

#include "common/extension.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "gssoui/gssoui_dbus_glue.h"

class GssouiData;

class GssouiServer {
  struct RequestData {
    GSSODBusUIDialog *dialog;
    GDBusMethodInvocation* context;
    GssouiData *params;

    RequestData(GSSODBusUIDialog* d, GDBusMethodInvocation* c, GssouiData* data)
        : dialog(d), context(c), params(data) {
    }

   private:
    DISALLOW_COPY_AND_ASSIGN(RequestData);
  };

 public:
  GssouiServer();
  virtual ~GssouiServer();
  void SetActiveInstance(common::Instance *instance) {
    active_instance_ = instance;
  }

 private:
  void StartDbusServer();
  void StopDbusServer();
  void StartHttpServer();
  void StopHttpServer();
  bool QueueDialogRequest(GSSODBusUIDialog* dialog,
                          GDBusMethodInvocation* context,
                          GssouiData* data);
  void EndRequest(const GssouiData& result);
  void HandleHttpRequest(SoupMessage* message,
                         const char* path,
                         GHashTable* query,
                         SoupClientContext* client);
  bool ProcessNextRequest();

  void PostMessage(const picojson::value &msg) {
    if (!active_instance_) return;
    active_instance_->PostMessage(msg.serialize().c_str());
  }

  void PostMessage(const picojson::value::object &msg) {
    PostMessage(picojson::value(msg));
  }

  //
  // Static methods(c callbacks)
  //
  static void BusAcquiredCb(GDBusConnection* connection,
                            const gchar* name,
                            gpointer userdata);
  static void NameAcquiredCb(GDBusConnection* connection,
                             const gchar* name,
                             gpointer userdata);
  static void NameLostCb(GDBusConnection* connection,
                         const gchar* name,
                         gpointer userdata);
  static void ClientDisconnectedCb(GDBusConnection* conneciton,
                                   gboolean peer_vanished,
                                   GError* error,
                                   gpointer userdata);
  static gboolean ClientConnectedCb(GDBusServer* server,
                                    GDBusConnection* conneciton,
                                    gpointer userdata);
  static gboolean GetBusAddressCb(GSSODBusUI* ui,
                                  GDBusMethodInvocation* caller,
                                  gpointer userdata);
  static gboolean ShowDialogCb(GSSODBusUIDialog* dialog,
                               GDBusMethodInvocation* caller,
                               GVariant* params,
                               gpointer userdata);
  static gboolean RefreshDialogCb(GSSODBusUIDialog* dialog,
                                  GDBusMethodInvocation* caller,
                                  GVariant* params,
                                  gpointer userdata);
  static gboolean CancelDialogCb(GSSODBusUIDialog* dialog,
                                 GDBusMethodInvocation* caller,
                                 const gchar* req_id,
                                 gpointer userdata);
  static void HttpServerCb(SoupServer* server,
                           SoupMessage* message,
                           const char* path,
                           GHashTable* query,
                           SoupClientContext* client,
                           gpointer userdata);
  static void ForeachCb(gpointer key,
                        gpointer value,
                        gpointer userdata);

  common::Instance* active_instance_;
  guint dbus_owner_id_;
  gchar* server_socket_file_;
  GDBusServer* server_;
  GSSODBusUI* ui_;
  SoupServer* http_server_;
  std::queue<RequestData*> dialog_requests_;
  RequestData* active_request_;
  GHashTable* dialogs_;
};

#endif  // GSSOUI_GSSOUI_SERVER_H_

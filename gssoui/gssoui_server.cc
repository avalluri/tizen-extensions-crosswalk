// Copyright(c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gssoui/gssoui_server.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <string>

#include "gssoui/gssoui_data.h"
#include "gssoui/gssoui_logs.h"

const gchar kGssouiBusName[] = "com.google.code.AccountsSSO.gSingleSignOn.UI";
const guint kPortNumber = 9004;

//static 
void GssouiServer::ForeachCb(
    gpointer key,
    gpointer value,
    gpointer userdata) {
   g_signal_handlers_disconnect_by_func(
      key,
      (gpointer)GssouiServer::ClientDisconnectedCb,
      userdata);
   g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(value));
}

// static
void GssouiServer::ClientDisconnectedCb(
    GDBusConnection* connection,
    gboolean /*peer_vanished*/,
    GError* error,
    gpointer userdata) {
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);

  g_signal_handlers_disconnect_by_func(
      connection,
      (gpointer)GssouiServer::ClientDisconnectedCb,
      userdata);

  GDBusInterfaceSkeleton *skel = reinterpret_cast<GDBusInterfaceSkeleton*>(
      g_hash_table_lookup(self->dialogs_,
      reinterpret_cast<gpointer>(connection)));
  if (skel) {
    g_dbus_interface_skeleton_unexport(skel);
  }
  g_hash_table_remove(self->dialogs_, connection);
}

// static
void GssouiServer::HttpServerCb(
    SoupServer* server,
    SoupMessage* message,
    const char* path,
    GHashTable* query,
    SoupClientContext* client,
    gpointer userdata) {
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);

  self->HandleHttpRequest(message, path, query, client);
}

// static
gboolean GssouiServer::ShowDialogCb(
    GSSODBusUIDialog* dialog,
    GDBusMethodInvocation* caller_info,
    GVariant* params,
    gpointer userdata) {
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);

  self->QueueDialogRequest(dialog, caller_info, new GssouiData(params));
  return TRUE;
}

// static
gboolean GssouiServer::RefreshDialogCb(
    GSSODBusUIDialog* dialog,
    GDBusMethodInvocation* caller,
    GVariant* params,
    gpointer userdata) {
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);

  return TRUE;
}

// static
gboolean GssouiServer::CancelDialogCb(
    GSSODBusUIDialog* dialog,
    GDBusMethodInvocation* caller,
    const gchar* req_id,
    gpointer userdata) {
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);

  return TRUE;
}

// static
gboolean GssouiServer::ClientConnectedCb(
    GDBusServer* server,
    GDBusConnection* connection,
    gpointer userdata) {
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);

#if ENABLE_DEBUG
  GCredentials* c = g_dbus_connection_get_peer_credentials(connection);
  DBG("Client(pid-'%d') connected.",
      c ? g_credentials_get_unix_pid(c, NULL) : -1);
#endif  // ENABLE_DEBUG

  /* Export Dialog interface */
  GError *error = NULL;
  GSSODBusUIDialog* dialog = gsso_dbus_uidialog_skeleton_new();
  g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(dialog),
                                   connection, "/Dialog", &error);
  if (error) {
    WARN("Failed to export dialog service on connection: %s", error->message);
    g_error_free(error);
    g_clear_object(&dialog);

    return FALSE;
  }

  g_signal_connect(dialog, "handle-query-dialog",
                   G_CALLBACK(ShowDialogCb), self);
  g_signal_connect(dialog, "handle-refresh-dialog",
                   G_CALLBACK(RefreshDialogCb), self);
  g_signal_connect(dialog, "handle-cancel-ui-request",
                   G_CALLBACK(CancelDialogCb), self);

  g_hash_table_insert(self->dialogs_, g_object_ref(connection), dialog);

  g_signal_connect(connection, "closed",
                   G_CALLBACK(ClientDisconnectedCb), self);

  return TRUE;
}

// static
gboolean GssouiServer::GetBusAddressCb(
    GSSODBusUI* ui,
    GDBusMethodInvocation* caller,
    gpointer userdata) {
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);

  DBG("In GetBusAddress..");
  if (!self->server_) {
    self->StartDbusServer();
  }

  if (self->server_) {
    const char *address = g_dbus_server_get_client_address(self->server_);
    gsso_dbus_ui_complete_get_bus_address(ui, caller, address);
  } else {
    // FIXME(avalluri): define dbus errors
    //   g_dbus_method_invocation_take_error(caller,
    //       g_error_new());
  }

  return TRUE;
}

//static
void GssouiServer::NameAcquiredCb(
    GDBusConnection* /*connection*/,
    const gchar* /*name*/,
    gpointer /*userdata*/) {
  DBG("%s","D-Bus name acquired");
}

// static
void GssouiServer::NameLostCb(
    GDBusConnection* /*connection*/,
    const gchar* /*name*/,
    gpointer userdata) {
  DBG("%s", "D-Bus name lost");
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);
  g_clear_object(&self->server_);
}

// static
void GssouiServer::BusAcquiredCb(
    GDBusConnection* connection,
    const gchar* /*name*/,
    gpointer userdata) {
  GssouiServer* self = reinterpret_cast<GssouiServer*>(userdata);
  GError* error = NULL;

  DBG("D-Bus bus acquired");

  // expose interface
  if(!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(self->ui_),
                                       connection, "/", &error)) {
    WARN("Failed to export interface: %s", error->message);
    g_error_free(error);
  }
}

GssouiServer::GssouiServer()
    : dbus_owner_id_(0),
      ui_(gsso_dbus_ui_skeleton_new()),
      server_(NULL),
      http_server_(NULL),
      active_request_(NULL),
      dialogs_(g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                     (GDestroyNotify)g_object_unref,
                                     (GDestroyNotify)g_object_unref)) {
  DBG("New instace..");
  GBusNameOwnerFlags flags = static_cast<GBusNameOwnerFlags>(
      G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
      G_BUS_NAME_OWNER_FLAGS_REPLACE);
  dbus_owner_id_ = g_bus_own_name(G_BUS_TYPE_SESSION, kGssouiBusName, flags,
                                  BusAcquiredCb, NameAcquiredCb, NameLostCb,
                                  this, NULL);
  g_signal_connect(ui_, "handle-get-bus-address",
                   G_CALLBACK(GetBusAddressCb), this);
  StartHttpServer();
}

GssouiServer::~GssouiServer() {
  g_hash_table_foreach(dialogs_, ForeachCb, this);
  g_hash_table_unref(dialogs_);
  g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(ui_));
  g_clear_object(&ui_);
  g_bus_unown_name(dbus_owner_id_);
  
  StopDbusServer();
  StopHttpServer();
}

void GssouiServer::StartDbusServer() {
  DBG("In Start server ...");
  if (!server_socket_file_) {
    gchar* base_path = g_strdup_printf("%s/gsignond/",
                                       g_get_user_runtime_dir());

    server_socket_file_ = g_strdup_printf("%s%s", base_path, "ui-onetwothree");
 
    if(g_file_test(server_socket_file_, G_FILE_TEST_EXISTS)) {
      // gdbus server needs only non-existing file path
      g_unlink(server_socket_file_);
    }
    else {
      if(g_mkdir_with_parents(base_path, S_IRUSR | S_IWUSR | S_IXUSR) == -1) {
        WARN("Could not create '%s', error: %s", base_path, strerror(errno));
        g_free(base_path);
        g_free(server_socket_file_);
        server_socket_file_ = NULL;
        return;
      }
    }
    g_free(base_path);
  }

  DBG("Server socket : %s", server_socket_file_);

  gchar* address = g_strdup_printf("unix:path=%s", server_socket_file_);
  gchar* guid = g_dbus_generate_guid();
  GError* error = NULL;
  server_ = g_dbus_server_new_sync(address, G_DBUS_SERVER_FLAGS_NONE,
                                   guid, NULL, NULL, &error);
  g_free(guid);
  g_free(address);

  if(!server_) {
    WARN("Could not start dbus server at address '%s' : %s", address,
         error->message);
    g_error_free(error);

    g_free(server_socket_file_);
    server_socket_file_ = NULL;
    return ;
  }

  g_chmod(server_socket_file_, S_IRUSR | S_IWUSR);
  g_signal_connect(server_, "new-connection",
                   G_CALLBACK(ClientConnectedCb), this);
  g_dbus_server_start(server_);

  DBG("UI Dialog server started at : %s",
      g_dbus_server_get_client_address(server_));
}

void GssouiServer::StopDbusServer() {
  DBG("Stopping server ....");
  g_clear_object(&server_);
  if (server_socket_file_) {
    unlink(server_socket_file_);
    g_free(server_socket_file_);
  }
}

void GssouiServer::StartHttpServer() {
  if(http_server_) return;

  DBG("Starting http server...");

  // start http server to listen for authentication results
  http_server_ = soup_server_new(SOUP_SERVER_PORT, kPortNumber, NULL);
  soup_server_add_handler(http_server_, "/", GssouiServer::HttpServerCb,
                          this, NULL);
  soup_server_run_async(http_server_);
}

void GssouiServer::StopHttpServer()  {
  if (!http_server_) return;

  DBG("Stopping Http server...");

  soup_server_quit(http_server_);
  soup_server_disconnect(http_server_);
  g_clear_object(&http_server_);
}

void GssouiServer::EndRequest(const GssouiData& result) {
  if (!active_request_)
    return;
#ifndef NDEBUG
  result.Dump("Ending query request with reply");
#endif // NDEBUG
  gsso_dbus_uidialog_complete_query_dialog(active_request_->dialog,
                                           active_request_->context,
                                           result.ToVariant());
  picojson::object js_message;
  js_message["cmd"] = picojson::value("CloseDialog");
  PostMessage(js_message);

  delete active_request_;
  active_request_ = NULL;
}

bool GssouiServer::QueueDialogRequest(
    GSSODBusUIDialog* dialog,
    GDBusMethodInvocation* context,
    GssouiData* params) {
  if (!params->HasKey(kUiKeyRequestId))
    return FALSE;

  DBG("queuing up new dialog request");
  dialog_requests_.push(new RequestData(dialog, context, params));

  if (!active_request_)
    ProcessNextRequest();
   
  return TRUE;
}

bool GssouiServer::ProcessNextRequest() {
  if (active_request_) {
    WARN("active request is being processed, cannot process next request.");
    return false;
  }
  
  if (dialog_requests_.empty()) {
    DBG("No dialogs in queue");
    return false;
  }
  
  active_request_ = dialog_requests_.front();
  dialog_requests_.pop();

  picojson::object js_message;
  picojson::object js_params;
  GssouiData* params = active_request_->params;
  if (!params->HasKey(kUiKeyRequestId) ||
      !(params->HasKey(kUiKeyQueryPassword) ||
        params->HasKey(kUiKeyQueryUsername) ||
        params->HasKey(kUiKeyOpenUrl)) ||
      (params->HasKey(kUiKeyOpenUrl) && !params->HasKey(kUiKeyFinalUrl))) {
#ifndef NDEBUG
    params->Dump("Invalid request parameters");
#endif  // NDEBUG
    GssouiData reply;
    reply.Add(kUiKeyErrorCode,
              g_variant_new_uint32(GssouiData::Error::kBadParameters)); 
    EndRequest(reply);
    return ProcessNextRequest();
 }

  js_params[kUiKeyRequestId] =
      picojson::value(params->GetString(kUiKeyRequestId));
  if (params->HasKey(kUiKeyOpenUrl)) {
    js_params[kUiKeyOpenUrl] =
        picojson::value(params->GetString(kUiKeyOpenUrl));
  } else {
    if (params->HasKey(kUiKeyQueryUsername)) {
      js_params[kUiKeyQueryUsername] = picojson::value(true);
    }

    if (params->HasKey(kUiKeyQueryPassword)) {
      js_params[kUiKeyQueryPassword] = picojson::value(true);
      js_params[kUiKeyUserName] =
          picojson::value(params->GetString(kUiKeyUserName));
    }

    if (params->HasKey(kUiKeyRememberPassword))
      js_params[kUiKeyRememberPassword] = picojson::value(true);
    if (params->HasKey(kUiKeyTitle))
      js_params[kUiKeyTitle] = picojson::value(params->GetString(kUiKeyTitle));
    if (params->HasKey(kUiKeyCaption))
      js_params[kUiKeyCaption] =
          picojson::value(params->GetString(kUiKeyCaption));
    if (params->HasKey(kUiKeyCaption))
      js_params[kUiKeyCaption] =
          picojson::value(params->GetString(kUiKeyCaption));
  }

  js_message["cmd"] = picojson::value("ShowDialog");
  js_message["params"] = picojson::value(js_params);

  DBG("Signal to show dialog..");
  PostMessage(js_message);

  return true;
}

void GssouiServer::HandleHttpRequest(
    SoupMessage* message,
    const char* path,
    GHashTable* query,
    SoupClientContext* client) {
  const char* uri = soup_uri_to_string(soup_message_get_uri(message), FALSE);
  DBG("Authentication reply : %s", uri);

  if (strcmp(path, "/") != 0)
    return;

  GssouiData reply;
  if (active_request_->params->HasKey(kUiKeyOpenUrl)) {
    reply.Add(kUiKeyErrorCode, g_variant_new_uint32(
        GssouiData::Error::kErroNone));
    reply.Add(kUiKeyUrlResponse, g_variant_new_string(uri));
  } else {
    bool null_query = false;
    if(!query) {
      null_query = true;
      query  = soup_form_decode_urlencoded(soup_uri_get_query(
          soup_message_get_uri(message)));
    }

    if (g_hash_table_contains(query, (gpointer)"cancled")) {
        reply.Add(kUiKeyErrorCode, g_variant_new_uint32(
          GssouiData::Error::kCancled));
    } else {
      const char* username = reinterpret_cast<const char*>(
          g_hash_table_lookup(query, (gpointer)"username"));
      const char* secret = reinterpret_cast<const char*>(
          g_hash_table_lookup(query, (gpointer)"password"));
      bool remember = std::string(reinterpret_cast<const gchar*>(
          g_hash_table_lookup(query, (gpointer)"remember_password"))) == "true";
 
      reply.Add(kUiKeyErrorCode, g_variant_new_uint32(
          GssouiData::Error::kErroNone));
      reply.Add(kUiKeyUserName, g_variant_new_string(username));
      reply.Add(kUiKeySecret, g_variant_new_string(secret));
      reply.Add(kUiKeyRememberPassword, g_variant_new_boolean(remember));
    }
    if (null_query) g_hash_table_destroy(query);
  }

  std::string response = "\
      <html><head></head>\
      <body><script>\
      window.onload = function() {\
        console.log('Window loaded ....');\
        window.close();\
      };\
      </script>\
      <p>Authentication successfull. You can close the window to continue.</body></html>";

  soup_message_set_status(message, SOUP_STATUS_OK);
  soup_message_set_response(message, "text/html", SOUP_MEMORY_COPY,
                            response.c_str(), response.length());
  EndRequest(reply);
  ProcessNextRequest();
}

#include "ping_instance.h"

#include <gio/gio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "common/picojson.h"
#include "ping_logs.h"

std::ofstream _as_f_log;

#define WINTHORP_SERVER_SOCKET "@winthorpe.w3c-speech"

PingInstance::PingInstance()
    : timeout_source_id_(0)
    , watcher_id_(0)
    , count_(3) {
  LOG_INIT();
  DBG("Thread ID: %u", std::this_thread::get_id());

  if ((fd_ = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    ERR("Failed to create socket: %s", strerror(errno));
    fd_ = -1;
    return;
  }

  struct sockaddr_un server;
  memset(&server, 0, sizeof(server));
  server.sun_family = AF_UNIX,
    strncpy(server.sun_path, WINTHORP_SERVER_SOCKET, sizeof(server.sun_path) - 1);

  int len = SUN_LEN(&server);
  DBG("Socket path : %s", server.sun_path + 1);
  server.sun_path[0] = 0;

  if (connect(fd_, (struct sockaddr *)&server, len)) {
    ERR("Failed to connect to server : %s", strerror(errno));
    close(fd_);
    fd_ = -1;
    return;
  }

  channel_ = g_io_channel_unix_new(fd_);
  EnableIOWatch();
}

// static
gboolean PingInstance::IOWatchCb(GIOChannel *c,
    GIOCondition cond,
    gpointer userdata) {
  PingInstance *self = reinterpret_cast<PingInstance*>(userdata);
  DBG("Thread ID: %u", std::this_thread::get_id());

  (void)c;

  switch (cond) {
    case G_IO_HUP:
    case G_IO_ERR:
      // TODO(avalluri): raise error and close the connection
      break;
    case G_IO_IN: {
      char *reply = NULL;
      uint32_t size = 0;

      if ((size = self->ReadReply(&reply))) {
        self->PostMessage(reply);
        free(reply);
      }
      break;
    }
    default:
      break;
  }

  return TRUE;
}

bool PingInstance::SendRequest(const char *message) {
  uint32_t size = ((uint32_t)strlen(message));
  uint32_t size_be = htobe32(size);

  if (fd_ == -1) {
    ERR("Socket not connected!");
    return false;
  }

  if (send(fd_, static_cast<void*>(&size_be), sizeof(size_be), 0) < 0) {
    WARN("Failed to send message size: %s", strerror(errno));
    return false;
  }

  void *buf = const_cast<void*>(static_cast<const void *>(message));
  ssize_t len = 0;
  while (size && (len = send(fd_, buf, size, 0)) < (ssize_t)size) {
    if (len < 0) {
      WARN("Failed to send message to server: %s", strerror(errno));
      return false;
    }
    size -= len;
    buf = static_cast<char*>(buf) + len;
  }

  return true;
}


uint32_t PingInstance::ReadReply(char **reply) {
  uint32_t size_be = 0;

  if (recv(fd_, static_cast<void*>(&size_be),
        sizeof(size_be), MSG_WAITALL) < 0) {
    WARN("Failed read server reply: %s", strerror(errno));
    return 0;
  }

  uint32_t size = be32toh(size_be);
  DBG("Received message size : %u", size);
  char *message = static_cast<char *>(malloc(size + 1));
  memset(message, 0, size);

  // FIXME: loop through till complete read
  if (recv(fd_, message, size, MSG_WAITALL) < 0) {
    WARN("Failed to read server message with size '%u': %s",
        size, strerror(errno));
    free(message);
    return 0;
  }
  message[size] = '\0';

  DBG("Recived message : %s", message);

  if (reply) *reply = message;

  return size;
}

void PingInstance::EnableIOWatch() {
  if (channel_ && !watcher_id_) {
    GIOCondition flags = GIOCondition(G_IO_IN | G_IO_ERR | G_IO_HUP);
    watcher_id_ = g_io_add_watch(channel_, flags, IOWatchCb, this);
  }
}

void PingInstance::DisableIOWatch() {
  if (channel_ && watcher_id_) {
    g_source_remove(watcher_id_);
    watcher_id_ = 0;
  }
}

PingInstance::~PingInstance() {
  DBG("");
  if (timeout_source_id_) {
    g_source_remove(timeout_source_id_);
    timeout_source_id_ = 0;
  }
}

// static
gboolean PingInstance::TimeoutCb(gpointer data) {
  DBG("Timeout Thread ID: %u", std::this_thread::get_id());
  PingInstance *self = reinterpret_cast<PingInstance*>(data);
  picojson::object pong;

  pong["reply"] = picojson::value("pong");

  self->PostMessage(picojson::value(pong).serialize().c_str());
  if (--self->count_ == 0) {
    DBG("Closing Timer : %u", self->timeout_source_id_);
    self->timeout_source_id_ = 0;
    return FALSE;
  }

  return TRUE;
}

void PingInstance::HandleMessage(const char *message) {
  picojson::value msg;
  std::string err;

  //DisableIOWatch();

  picojson::parse(msg, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "ERROR:" << err;
  }
  std::string req = msg.get("request").to_str();
  if (req == "start") {
    guint timeout = static_cast<guint>(msg.get("timeout").get<double>());
    if (!timeout_source_id_) {
      count_ = 3;
      DBG("Starting Timer....");
      timeout_source_id_ = g_timeout_add(timeout, TimeoutCb, static_cast<gpointer>(this));
      DBG("Timer Source id : %u", timeout_source_id_);
    }
  } else if (req == "stop") {
    if (timeout_source_id_) {
      DBG("Stopping Timer....");
      g_source_remove (timeout_source_id_);
      timeout_source_id_ = 0;
    }
  } else {
    std::cout <<"ERROR: unknown request : " << req;
  }

  //EnableIOWatch();
}

void PingInstance::HandleSyncMessage(const char *message) {
  uint32_t size;
  char *reply = NULL;
  std::string err;
  picojson::value out;

  DisableIOWatch();
  SendRequest(message);
  if ((size = ReadReply(&reply)) != 0) {
    picojson::parse(out, reply, reply + size, &err);
    free(reply);
  }
  EnableIOWatch();
  SendSyncReply(out.serialize().c_str());
}

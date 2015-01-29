#ifndef TIZEN_PING_INSTANCE_H_
#define TIZEN_PING_INSTANCE_H_

#include <glib.h>

#include "common/extension.h"

class PingInstance : public common::Instance {
 public:
  PingInstance();
  ~PingInstance();
 protected:
  static gboolean TimeoutCb(gpointer data);
  static gboolean IOWatchCb(GIOChannel* source,
                              GIOCondition condition, gpointer data); 

 private:
  virtual void HandleMessage(const char *message);
  virtual void HandleSyncMessage(const char *message);

  void EnableIOWatch();
  void DisableIOWatch();
  uint32_t ReadReply(char** reply);
   bool SendRequest(const char *message);

  int fd_;
  guint32 timeout_source_id_;
  int count_;
  GIOChannel *channel_;
  guint watcher_id_;
};


#endif  // TIZEN_PING_INSTANCE_H_

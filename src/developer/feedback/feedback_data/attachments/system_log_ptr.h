// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_ATTACHMENTS_SYSTEM_LOG_PTR_H_
#define SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_ATTACHMENTS_SYSTEM_LOG_PTR_H_

#include <fuchsia/logger/cpp/fidl.h>
#include <lib/async/dispatcher.h>
#include <lib/fidl/cpp/binding.h>
#include <lib/fit/promise.h>
#include <lib/sys/cpp/service_directory.h>

#include <cinttypes>
#include <vector>

#include "src/developer/feedback/feedback_data/attachments/aliases.h"
#include "src/developer/feedback/utils/fidl/oneshot_ptr.h"
#include "src/developer/feedback/utils/fit/timeout.h"

namespace feedback {

// Collects system log.
//
// fuchsia.logger.Log is expected to be in |services|.
::fit::promise<AttachmentValue> CollectSystemLog(async_dispatcher_t* dispatcher,
                                                 std::shared_ptr<sys::ServiceDirectory> services,
                                                 fit::Timeout timeout);

// Wraps around fuchsia::logger::LogListenerSafePtr to handle establishing the connection, losing
// the connection, waiting for the callback, enforcing a timeout, etc.
//
// CollectLogs() is expected to be called only once.
class LogListener : public fuchsia::logger::LogListenerSafe {
 public:
  explicit LogListener(async_dispatcher_t* dispatcher,
                       std::shared_ptr<sys::ServiceDirectory> services);

  // Collects the logs and returns a promise to when the collection is done or the timeout over.
  ::fit::promise<void> CollectLogs(fit::Timeout timeout);

  // Returns the logs that have been collected so far.
  std::string CurrentLogs() { return logs_; }

 private:
  // |fuchsia::logger::LogListenerSafe|
  void LogMany(::std::vector<fuchsia::logger::LogMessage> log, LogManyCallback done) override;
  void Log(fuchsia::logger::LogMessage log, LogCallback done) override;
  void Done() override;

  ::fidl::Binding<fuchsia::logger::LogListenerSafe> binding_;

  fidl::OneShotPtr<fuchsia::logger::Log> logger_;

  // Wether LogMany() was called since the last call to CollectLogs().
  // This is to help debug FLK-179.
  bool log_many_called_ = false;

  std::string logs_;
};

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_ATTACHMENTS_SYSTEM_LOG_PTR_H_

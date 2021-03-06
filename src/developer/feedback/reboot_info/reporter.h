// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_REBOOT_INFO_REPORTER_H_
#define SRC_DEVELOPER_FEEDBACK_REBOOT_INFO_REPORTER_H_

#include <fuchsia/feedback/cpp/fidl.h>
#include <lib/async/cpp/executor.h>
#include <lib/fit/promise.h>
#include <lib/sys/cpp/service_directory.h>
#include <lib/zx/time.h>

#include <memory>
#include <string>

#include "src/developer/feedback/reboot_info/reboot_log.h"
#include "src/developer/feedback/utils/cobalt/logger.h"
#include "src/developer/feedback/utils/fidl/oneshot_ptr.h"
#include "src/lib/fxl/functional/cancelable_callback.h"

namespace feedback {

// Logs the reboot reason with Cobalt and if the reboot was non-graceful, files a crash report.
class Reporter {
 public:
  // fuchsia.feedback.CrashReporter and fuchsia.cobalt.LoggerFactory are expected to be in
  // |services|.
  Reporter(async_dispatcher_t* dispatcher, std::shared_ptr<sys::ServiceDirectory> services);

  void ReportOn(const RebootLog& reboot_log, zx::duration crash_reporting_delay);

 private:
  ::fit::promise<void> FileCrashReport(const RebootLog& reboot_log, zx::duration delay);

  async_dispatcher_t* dispatcher_;
  async::Executor executor_;

  fidl::OneShotPtr<fuchsia::feedback::CrashReporter> crash_reporter_;
  cobalt::Logger cobalt_;

  // We wrap the delayed task we post on the async loop to delay the crash reporting in a
  // CancelableClosure so we can cancel it if we are done another way.
  fxl::CancelableClosure delayed_crash_reporting_;
};

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_REBOOT_INFO_REPORTER_H_

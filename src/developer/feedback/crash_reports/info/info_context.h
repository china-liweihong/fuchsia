// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_CRASH_REPORTS_INFO_INFO_CONTEXT_H_
#define SRC_DEVELOPER_FEEDBACK_CRASH_REPORTS_INFO_INFO_CONTEXT_H_

#include <lib/async/dispatcher.h>
#include <lib/inspect/cpp/vmo/types.h>
#include <lib/sys/cpp/service_directory.h>
#include <lib/timekeeper/clock.h>

#include <memory>

#include "src/developer/feedback/crash_reports/info/inspect_manager.h"
#include "src/developer/feedback/utils/cobalt/logger.h"

namespace feedback {

// Holds the objects needed to expose information for a component.
class InfoContext {
 public:
  InfoContext(inspect::Node *root_node, const timekeeper::Clock &clock,
              async_dispatcher_t *dispatcher, std::shared_ptr<sys::ServiceDirectory> services)
      : inspect_manager_(root_node, clock), cobalt_(dispatcher, services) {}

  InspectManager &InspectManager() { return inspect_manager_; }
  cobalt::Logger &Cobalt() { return cobalt_; }

 private:
  class InspectManager inspect_manager_;
  class cobalt::Logger cobalt_;
};

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_CRASH_REPORTS_INFO_INFO_CONTEXT_H_

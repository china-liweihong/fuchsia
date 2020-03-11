// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_FEEDBACK_AGENT_DATA_REGISTER_H_
#define SRC_DEVELOPER_FEEDBACK_FEEDBACK_AGENT_DATA_REGISTER_H_

#include <fuchsia/feedback/cpp/fidl.h>

namespace feedback {

// Registers data useful to attach in feedback reports (crash, user feedback or bug reports).
class DataRegister : public fuchsia::feedback::ComponentDataRegister {
 public:
  DataRegister() = default;

  // |fuchsia.feedback.ComponentDataRegister|
  void Upsert(fuchsia::feedback::ComponentData data, UpsertCallback callback);
};

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_FEEDBACK_AGENT_DATA_REGISTER_H_
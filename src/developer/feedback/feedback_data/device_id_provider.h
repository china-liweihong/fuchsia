// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_DEVICE_ID_PROVIDER_H_
#define SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_DEVICE_ID_PROVIDER_H_

#include <fuchsia/feedback/cpp/fidl.h>

#include <string>

#include "src/developer/feedback/feedback_data/annotations/types.h"

namespace feedback {

// Manages and provides the device id at the provided path.
class DeviceIdProvider : public fuchsia::feedback::DeviceIdProvider {
 public:
  DeviceIdProvider(const std::string& path);

  AnnotationOr GetId();

  // |fuchsia.feedback.DeviceIdProvider|
  void GetId(GetIdCallback callback) override;

 private:
  AnnotationOr device_id_;
};

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_DEVICE_ID_PROVIDER_H_

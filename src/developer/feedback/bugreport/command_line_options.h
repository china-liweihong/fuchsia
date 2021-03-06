// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_BUGREPORT_COMMAND_LINE_OPTIONS_H_
#define SRC_DEVELOPER_FEEDBACK_BUGREPORT_COMMAND_LINE_OPTIONS_H_

namespace feedback {

enum class Mode {
  FAILURE,
  HELP,
  DEFAULT,
};

Mode ParseModeFromArgcArgv(int argc, const char* const* argv);

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_BUGREPORT_COMMAND_LINE_OPTIONS_H_

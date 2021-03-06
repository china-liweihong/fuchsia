// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "garnet/bin/trace/tests/component_context.h"
#include "src/lib/fxl/command_line.h"
#include "src/lib/fxl/log_settings.h"
#include "src/lib/fxl/log_settings_command_line.h"
#include "src/lib/fxl/logging.h"
#include "src/lib/fxl/test/test_settings.h"

fxl::LogSettings g_log_settings;

int main(int argc, char** argv) {
  auto cl = fxl::CommandLineFromArgcArgv(argc, argv);
  if (!fxl::SetTestSettings(cl)) {
    FX_LOGS(ERROR) << "Failed to parse log settings from command-line";
    return EXIT_FAILURE;
  }
  fxl::ParseLogSettings(cl, &g_log_settings);

  testing::InitGoogleTest(&argc, argv);

  tracing::test::InitComponentContext();

  return RUN_ALL_TESTS();
}

// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/sys/cpp/component_context.h>

#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

#include "src/developer/feedback/bugreport/bug_reporter.h"
#include "src/developer/feedback/bugreport/command_line_options.h"

namespace {

using feedback::Mode;

}  // namespace

int main(int argc, char** argv) {
  const Mode mode = feedback::ParseModeFromArgcArgv(argc, argv);
  std::set<std::string> attachment_allowlist;
  switch (mode) {
    case Mode::FAILURE:
      return EXIT_FAILURE;
    case Mode::HELP:
      return EXIT_SUCCESS;
    case Mode::DEFAULT:
      auto environment_services = sys::ServiceDirectory::CreateFromNamespace();

      return feedback::MakeBugReport(environment_services) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
}

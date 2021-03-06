// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/fdio/spawn.h>
#include <stdlib.h>
#include <zircon/process.h>
#include <zircon/status.h>
#include <zircon/syscalls.h>

#include <gtest/gtest.h>

#include "src/lib/files/file.h"

static constexpr char kLogListenerPath[] = "/pkg/bin/log_listener";

TEST(LogListenerReturnCode, ReturnNonzeroOnBadArgs) {
  // Spawn log_listener with bad args
  uint32_t flags = FDIO_SPAWN_CLONE_ALL;
  const char** argv = new const char* [] { kLogListenerPath, "very", "invalid", "arguments", NULL };
  zx_handle_t process = ZX_HANDLE_INVALID;
  zx_status_t status = fdio_spawn(ZX_HANDLE_INVALID, flags, kLogListenerPath, argv, &process);
  ASSERT_EQ(ZX_OK, status);

  // Verify return code
  status = zx_object_wait_one(process, ZX_TASK_TERMINATED, ZX_TIME_INFINITE, NULL);
  ASSERT_EQ(ZX_OK, status);
  zx_info_process_t proc_info;
  status = zx_object_get_info(process, ZX_INFO_PROCESS, &proc_info, sizeof(proc_info), NULL, NULL);
  ASSERT_EQ(ZX_OK, status);
  ASSERT_NE(0, proc_info.return_code);
}

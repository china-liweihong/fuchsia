// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/feedback/cpp/fidl.h>
#include <lib/async-loop/cpp/loop.h>
#include <lib/async-loop/default.h>
#include <lib/fidl/cpp/binding_set.h>
#include <lib/sys/cpp/component_context.h>

#include "src/developer/feedback/testing/fakes/device_id_provider.h"
#include "src/lib/syslog/cpp/logger.h"

int main(int argc, const char** argv) {
  syslog::SetTags({"feedback", "test"});

  FX_LOGS(INFO) << "Starting FakeDeviceIdProvider";

  async::Loop loop(&kAsyncLoopConfigAttachToCurrentThread);
  auto context = sys::ComponentContext::CreateAndServeOutgoingDirectory();

  feedback::fakes::DeviceIdProvider device_id_provider;

  ::fidl::BindingSet<fuchsia::feedback::DeviceIdProvider> device_id_provider_bindings;
  context->outgoing()->AddPublicService(
      device_id_provider_bindings.GetHandler(&device_id_provider));

  loop.Run();

  return EXIT_SUCCESS;
}

// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/developer/feedback/testing/stubs/network_reachability_provider.h"

#include <zircon/errors.h>

#include "src/lib/syslog/cpp/logger.h"

namespace feedback {
namespace stubs {

void NetworkReachabilityProvider::TriggerOnNetworkReachable(const bool reachable) {
  FX_CHECK(binding()) << "No client is connected to the stub server yet";
  binding()->events().OnNetworkReachable(reachable);
}

}  // namespace stubs
}  // namespace feedback

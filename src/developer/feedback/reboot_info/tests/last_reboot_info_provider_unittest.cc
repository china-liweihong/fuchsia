// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/developer/feedback/reboot_info/last_reboot_info_provider.h"

#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/developer/feedback/reboot_info/last_reboot_info_provider.h"
#include "src/developer/feedback/reboot_info/reboot_log.h"
#include "src/developer/feedback/reboot_info/reboot_reason.h"
#include "src/developer/feedback/testing/gpretty_printers.h"

namespace feedback {
namespace {

fuchsia::feedback::LastReboot GetLastReboot(
    const RebootReason reboot_reason, const std::optional<zx::duration> uptime = std::nullopt) {
  const RebootLog reboot_log(reboot_reason, "", uptime);
  fuchsia::feedback::LastReboot out_last_reboot;

  LastRebootInfoProvider last_reboot_info_provider(reboot_log);
  last_reboot_info_provider.Get(
      [&](fuchsia::feedback::LastReboot last_reboot) { out_last_reboot = std::move(last_reboot); });

  return out_last_reboot;
}

TEST(LastRebootInfoProviderTest, Succeed_Graceful) {
  const RebootReason reboot_reason = RebootReason::kGenericGraceful;

  const auto last_reboot = GetLastReboot(reboot_reason);

  ASSERT_TRUE(last_reboot.has_graceful());
  EXPECT_TRUE(last_reboot.graceful());

  ASSERT_TRUE(last_reboot.has_reason());
  EXPECT_EQ(last_reboot.reason(), ToFidlRebootReason(reboot_reason));
}

TEST(LastRebootInfoProviderTest, Succeed_NotGraceful) {
  const RebootReason reboot_reason = RebootReason::kKernelPanic;

  const auto last_reboot = GetLastReboot(reboot_reason);

  ASSERT_TRUE(last_reboot.has_graceful());
  EXPECT_FALSE(last_reboot.graceful());

  ASSERT_TRUE(last_reboot.has_reason());
  EXPECT_EQ(last_reboot.reason(), ToFidlRebootReason(reboot_reason));
}

TEST(LastRebootInfoProviderTest, Succeed_HasUptime) {
  const zx::duration uptime = zx::msec(100);

  const auto last_reboot = GetLastReboot(RebootReason::kGenericGraceful, uptime);

  ASSERT_TRUE(last_reboot.has_uptime());
  EXPECT_EQ(last_reboot.uptime(), uptime.to_msecs());
}

TEST(LastRebootInfoProviderTest, Succeed_DoesNotHaveUptime) {
  const auto last_reboot = GetLastReboot(RebootReason::kGenericGraceful, std::nullopt);

  EXPECT_FALSE(last_reboot.has_uptime());
}

}  // namespace
}  // namespace feedback

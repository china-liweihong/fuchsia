// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "platform_random.h"

TEST(PlatformRandom, DifferentValues) {
  uint8_t buffer0[128] = {};
  uint8_t buffer1[128] = {};
  magma_platform_GetSecureRandomBytes(buffer0, sizeof(buffer0));
  magma_platform_GetSecureRandomBytes(buffer1, sizeof(buffer1));

  // This could fail by accident, but it's extremely unlikely.
  EXPECT_NE(0, memcmp(buffer0, buffer1, sizeof(buffer0)));
}

// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <zircon/types.h>

#include <ddk/driver.h>

#include "input.h"
#include "src/devices/bus/lib/virtio/driver_utils.h"

static const zx_driver_ops_t virtio_input_driver_ops = []() {
  zx_driver_ops_t ops = {};
  ops.version = DRIVER_OPS_VERSION;
  ops.bind = CreateAndBind<virtio::InputDevice>;
  return ops;
}();

ZIRCON_DRIVER_BEGIN(virtio_input, virtio_input_driver_ops, "zircon", "0.1", 5)
BI_ABORT_IF(NE, BIND_PROTOCOL, ZX_PROTOCOL_PCI),
    BI_ABORT_IF(NE, BIND_PCI_VID, VIRTIO_PCI_VENDOR_ID),
    BI_MATCH_IF(EQ, BIND_PCI_DID, VIRTIO_DEV_TYPE_INPUT), ZIRCON_DRIVER_END(virtio_input)

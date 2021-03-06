// Copyright 2020 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <zircon/assert.h>

#include "main.h"

// This is what ZX_ASSERT calls.
// TODO(mcgrathr): print message, backtrace
PHYS_SINGLETHREAD void __zx_panic(const char* format, ...) { __builtin_trap(); }

# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := userapp
MODULE_GROUP := core

# app main
MODULE_SRCS := \
    $(LOCAL_DIR)/main.cpp \

# "libfs"
MODULE_SRCS += \
    $(LOCAL_DIR)/bcache.cpp \

# minfs implementation
MODULE_SRCS += \
    $(LOCAL_DIR)/minfs.cpp \
    $(LOCAL_DIR)/minfs-ops.cpp \
    $(LOCAL_DIR)/minfs-check.cpp \

MODULE_STATIC_LIBS := \
    system/ulib/fs \
    system/ulib/async \
    system/ulib/async.loop \
    system/ulib/block-client \
    system/ulib/hash \
    system/ulib/zx \
    system/ulib/zxcpp \
    system/ulib/fbl \
    system/ulib/sync \

MODULE_LIBS := \
    system/ulib/async.default \
    system/ulib/bitmap \
    system/ulib/zircon \
    system/ulib/fdio \
    system/ulib/c \
    system/ulib/fdio \

include make/module.mk


# host minfs tool

MODULE := $(LOCAL_DIR).host

MODULE_NAME := minfs

MODULE_TYPE := hostapp

MODULE_SRCS := \
    $(LOCAL_DIR)/main.cpp \
    $(LOCAL_DIR)/host.cpp \
    $(LOCAL_DIR)/bcache.cpp \
    $(LOCAL_DIR)/minfs.cpp \
    $(LOCAL_DIR)/minfs-ops.cpp \
    system/ulib/bitmap/raw-bitmap.cpp \
    system/ulib/fs/vfs.cpp \
    system/ulib/fs/vnode.cpp \

MODULE_COMPILEFLAGS := \
    -Werror-implicit-function-declaration \
    -Wstrict-prototypes -Wwrite-strings \
    -Isystem/ulib/bitmap/include \
    -Isystem/ulib/zxcpp/include \
    -Isystem/ulib/fdio/include \
    -Isystem/ulib/fbl/include \
    -Isystem/ulib/fs/include \
    -Isystem/ulib/hash/include \

MODULE_HOST_LIBS := \
    system/ulib/fbl.hostlib

ifeq ($(HOST_PLATFORM),darwin)
MODULE_DEFINES := O_DIRECTORY=0200000
else
MODULE_DEFINES := _POSIX_C_SOURCE=200809L
endif

MODULE_DEFINES += DISABLE_THREAD_ANNOTATIONS

include make/module.mk

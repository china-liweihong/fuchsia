// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_CONSTANTS_H_
#define SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_CONSTANTS_H_

#include <fuchsia/feedback/cpp/fidl.h>

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace feedback {

///////////////////////////////////////////////////////////////////////////////////////////////////
// Annotations
///////////////////////////////////////////////////////////////////////////////////////////////////

const uint32_t kMaxNumPlatformAnnotations = 32u;
const uint32_t kMaxNumExtraAnnotations = 32u;
static_assert(kMaxNumPlatformAnnotations + kMaxNumExtraAnnotations ==
                  fuchsia::feedback::MAX_NUM_ANNOTATIONS_PROVIDED,
              "The max number of provided annotations has to be split between a max number of "
              "platform annotations and a max number of extra annotations");

// Platform annotation keys.
constexpr char kAnnotationBuildBoard[] = "build.board";
constexpr char kAnnotationBuildIsDebug[] = "build.is_debug";
constexpr char kAnnotationBuildLatestCommitDate[] = "build.latest-commit-date";
constexpr char kAnnotationBuildProduct[] = "build.product";
constexpr char kAnnotationBuildVersion[] = "build.version";
constexpr char kAnnotationDeviceBoardName[] = "device.board-name";
constexpr char kAnnotationDeviceFeedbackId[] = "device.feedback-id";
constexpr char kAnnotationDeviceUptime[] = "device.uptime";
constexpr char kAnnotationDeviceUTCTime[] = "device.utc-time";
constexpr char kAnnotationHardwareBoardName[] = "hardware.board.name";
constexpr char kAnnotationHardwareBoardRevision[] = "hardware.board.revision";
constexpr char kAnnotationHardwareProductLanguage[] = "hardware.product.language";
constexpr char kAnnotationHardwareProductLocaleList[] = "hardware.product.locale-list";
constexpr char kAnnotationHardwareProductManufacturer[] = "hardware.product.manufacturer";
constexpr char kAnnotationHardwareProductModel[] = "hardware.product.model";
constexpr char kAnnotationHardwareProductName[] = "hardware.product.name";
constexpr char kAnnotationHardwareProductRegulatoryDomain[] = "hardware.product.regulatory-domain";
constexpr char kAnnotationHardwareProductSKU[] = "hardware.product.sku";
constexpr char kAnnotationSystemUpdateChannelCurrent[] = "system.update-channel.current";

// Reserved namespaces for platform annotations. Components are not allowed to use these namespaces
// when supplying extra annotations.
const std::set<const std::string> kReservedAnnotationNamespaces({
    "build",
    "device",
    "hardware",
    "hardware.board",
    "hardware.product",
    "misc",
    "system",
});

///////////////////////////////////////////////////////////////////////////////////////////////////
// Attachments
///////////////////////////////////////////////////////////////////////////////////////////////////

// Platform attachments keys.
constexpr char kAttachmentAnnotations[] = "annotations.json";
constexpr char kAttachmentBuildSnapshot[] = "build.snapshot.xml";
constexpr char kAttachmentInspect[] = "inspect.json";
constexpr char kAttachmentLogKernel[] = "log.kernel.txt";
constexpr char kAttachmentLogSystem[] = "log.system.txt";
constexpr char kAttachmentLogSystemPrevious[] = "log.system.previous_boot.txt";

// Bugreport key.
constexpr char kAttachmentBundle[] = "bugreport.zip";

///////////////////////////////////////////////////////////////////////////////////////////////////
// Device ID
///////////////////////////////////////////////////////////////////////////////////////////////////

constexpr char kDeviceIdPath[] = "/data/device_id.txt";

///////////////////////////////////////////////////////////////////////////////////////////////////
// Logs from previous boot cycle.
///////////////////////////////////////////////////////////////////////////////////////////////////

constexpr char kPreviousLogsFilePath[] = "/tmp/log.system.previous_boot.txt";

// We use the 4 files below to store up to 512 kb of logs. So, assuming all components have logged
// at least 512 kb of data, we can expect between 384 kb and 512 kb of logs to be persisted due to
// the log rotation.
constexpr uint64_t kPersistentLogsMaxSizeInKb = 512;
const std::vector<const std::string> kCurrentLogsFilePaths({
    "/cache/current_system_log_0.txt",
    "/cache/current_system_log_1.txt",
    "/cache/current_system_log_2.txt",
    "/cache/current_system_log_3.txt",
});

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_FEEDBACK_DATA_CONSTANTS_H_

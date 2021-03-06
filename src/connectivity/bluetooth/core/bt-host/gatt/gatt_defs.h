// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_GATT_GATT_DEFS_H_
#define SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_GATT_GATT_DEFS_H_

#include "src/connectivity/bluetooth/core/bt-host/att/att.h"
#include "src/connectivity/bluetooth/core/bt-host/common/identifier.h"
#include "src/connectivity/bluetooth/core/bt-host/common/uuid.h"

namespace bt {
namespace gatt {

// 16-bit Attribute Types defined by the GATT profile (Vol 3, Part G, 3.4).
namespace types {

constexpr uint16_t kPrimaryService16 = 0x2800;
constexpr uint16_t kSecondaryService16 = 0x2801;
constexpr uint16_t kIncludeDeclaration16 = 0x2802;
constexpr uint16_t kCharacteristicDeclaration16 = 0x2803;
constexpr uint16_t kCharacteristicExtProperties16 = 0x2900;
constexpr uint16_t kCharacteristicUserDescription16 = 0x2901;
constexpr uint16_t kClientCharacteristicConfig16 = 0x2902;
constexpr uint16_t kServerCharacteristicConfig16 = 0x2903;
constexpr uint16_t kCharacteristicFormat16 = 0x2904;
constexpr uint16_t kCharacteristicAggregateFormat16 = 0x2905;
constexpr uint16_t kGenericAttributeService16 = 0x1801;
constexpr uint16_t kServiceChangedCharacteristic16 = 0x2a05;

constexpr UUID kPrimaryService(kPrimaryService16);
constexpr UUID kSecondaryService(kSecondaryService16);
constexpr UUID kIncludeDeclaration(kIncludeDeclaration16);
constexpr UUID kCharacteristicDeclaration(kCharacteristicDeclaration16);
constexpr UUID kCharacteristicExtProperties(kCharacteristicExtProperties16);
constexpr UUID kCharacteristicUserDescription(kCharacteristicUserDescription16);
constexpr UUID kClientCharacteristicConfig(kClientCharacteristicConfig16);
constexpr UUID kServerCharacteristicConfig(kServerCharacteristicConfig16);
constexpr UUID kCharacteristicFormat(kCharacteristicFormat16);
constexpr UUID kCharacteristicAggregateFormat(kCharacteristicAggregateFormat16);

// Defined Generic Attribute Profile Service (Vol 3, Part G, 7)
constexpr bt::UUID kGenericAttributeService(kGenericAttributeService16);
constexpr bt::UUID kServiceChangedCharacteristic(kServiceChangedCharacteristic16);

}  // namespace types

// Represents the reliability mode during long and prepared write operations.
enum ReliableMode {
  kDisabled = 0x01,
  kEnabled = 0x02,
};

// Possible values that can be used in a "Characteristic Properties" bitfield.
// (see Vol 3, Part G, 3.3.1.1)
enum Property : uint8_t {
  kBroadcast = 0x01,
  kRead = 0x02,
  kWriteWithoutResponse = 0x04,
  kWrite = 0x08,
  kNotify = 0x10,
  kIndicate = 0x20,
  kAuthenticatedSignedWrites = 0x40,
  kExtendedProperties = 0x80,
};
using Properties = uint8_t;

// Values for "Characteristic Extended Properties" bitfield.
// (see Vol 3, Part G, 3.3.3.1)
enum ExtendedProperty : uint16_t {
  kReliableWrite = 0x0001,
  kWritableAuxiliaries = 0x0002,
};
using ExtendedProperties = uint16_t;

// Values for the "Client Characteristic Configuration" descriptor.
constexpr uint16_t kCCCNotificationBit = 0x0001;
constexpr uint16_t kCCCIndicationBit = 0x0002;

using PeerId = PeerId;

// An identity for a Characteristic within a RemoteService
// Characteristic IDs are guaranteed to equal the Value Handle for the characteristic
struct CharacteristicHandle {
  constexpr explicit CharacteristicHandle(att::Handle handle) : value(handle) {}
  CharacteristicHandle() = delete;
  CharacteristicHandle(const CharacteristicHandle& other) = default;

  CharacteristicHandle& operator=(const CharacteristicHandle& other) = default;

  inline bool operator<(const CharacteristicHandle& rhs) const { return this->value < rhs.value; }
  inline bool operator==(const CharacteristicHandle& rhs) const { return this->value == rhs.value; }

  att::Handle value;
};

// Descriptors are identified by their underlying ATT handle
struct DescriptorHandle {
  DescriptorHandle(att::Handle handle) : value(handle) {}
  DescriptorHandle() = delete;
  DescriptorHandle(const DescriptorHandle& other) = default;

  DescriptorHandle& operator=(const DescriptorHandle& other) = default;

  inline bool operator<(const DescriptorHandle& rhs) const { return this->value < rhs.value; }
  inline bool operator==(const DescriptorHandle& rhs) const { return this->value == rhs.value; }

  att::Handle value;
};

// An identifier uniquely identifies a local GATT service, characteristic, or descriptor.
using IdType = uint64_t;

// 0 is reserved as an invalid ID.
constexpr IdType kInvalidId = 0u;

// Types representing GATT discovery results.

struct ServiceData {
  ServiceData() = default;
  ServiceData(att::Handle start, att::Handle end, const UUID& type);

  att::Handle range_start;
  att::Handle range_end;
  UUID type;
};

// An immutable definition of a GATT Characteristic
struct CharacteristicData {
  CharacteristicData() = delete;
  CharacteristicData(Properties props, std::optional<ExtendedProperties> ext_props,
                     att::Handle handle, att::Handle value_handle, const UUID& type);


  Properties properties;
  std::optional<ExtendedProperties> extended_properties;
  att::Handle handle;
  att::Handle value_handle;
  UUID type;
};

// An immutable definition of a GATT Descriptor
struct DescriptorData {
  DescriptorData() = delete;
  DescriptorData(att::Handle handle, const UUID& type);

  const att::Handle handle;
  const UUID type;
};

}  // namespace gatt
}  // namespace bt

// Specialization of std::hash for std::unordered_set, std::unordered_map, etc.
namespace std {

template <>
struct hash<bt::gatt::CharacteristicHandle> {
  size_t operator()(const bt::gatt::CharacteristicHandle& id) const {
    return std::hash<uint16_t>()(id.value);
  }
};
template <>
struct hash<bt::gatt::DescriptorHandle> {
  size_t operator()(const bt::gatt::DescriptorHandle& id) const {
    return std::hash<uint16_t>()(id.value);
  }
};

}  // namespace std

#endif  // SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_GATT_GATT_DEFS_H_

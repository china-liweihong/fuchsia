// Copyright 2020 the Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "phase_1.h"

#include <zircon/assert.h>

#include "src/connectivity/bluetooth/core/bt-host/common/byte_buffer.h"
#include "src/connectivity/bluetooth/core/bt-host/common/log.h"
#include "src/connectivity/bluetooth/core/bt-host/hci/connection.h"
#include "src/connectivity/bluetooth/core/bt-host/l2cap/scoped_channel.h"
#include "src/connectivity/bluetooth/core/bt-host/sm/packet.h"
#include "src/connectivity/bluetooth/core/bt-host/sm/smp.h"
#include "src/connectivity/bluetooth/core/bt-host/sm/types.h"
#include "src/connectivity/bluetooth/core/bt-host/sm/util.h"

namespace bt {
namespace sm {

std::unique_ptr<Phase1> Phase1::CreatePhase1Initiator(
    fxl::WeakPtr<PairingChannel> chan, fxl::WeakPtr<Listener> listener, IOCapability io_capability,
    BondableMode bondable_mode, bool mitm_required, CompleteCallback on_complete) {
  // Use `new` & unique_ptr constructor here instead of `std::make_unique` because the private
  // Phase1 constructor prevents std::make_unique from working (https://abseil.io/tips/134).
  return std::unique_ptr<Phase1>(
      new Phase1(std::move(chan), std::move(listener), hci::Connection::Role::kMaster, std::nullopt,
                 io_capability, bondable_mode, mitm_required, std::move(on_complete)));
}

std::unique_ptr<Phase1> Phase1::CreatePhase1Responder(
    fxl::WeakPtr<PairingChannel> chan, fxl::WeakPtr<Listener> listener, PairingRequestParams preq,
    IOCapability io_capability, BondableMode bondable_mode, bool mitm_required,
    CompleteCallback on_complete) {
  // Use `new` & unique_ptr constructor here instead of `std::make_unique` because the private
  // Phase1 constructor prevents std::make_unique from working (https://abseil.io/tips/134).
  return std::unique_ptr<Phase1>(new Phase1(std::move(chan), std::move(listener),
                                            hci::Connection::Role::kSlave, preq, io_capability,
                                            bondable_mode, mitm_required, std::move(on_complete)));
}

Phase1::Phase1(fxl::WeakPtr<PairingChannel> chan, fxl::WeakPtr<Listener> listener,
               hci::Connection::Role role, std::optional<PairingRequestParams> preq,
               IOCapability io_capability, BondableMode bondable_mode, bool mitm_required,
               CompleteCallback on_complete)
    : ActivePhase(std::move(chan), std::move(listener), role),
      peer_request_params_(preq),
      oob_available_(false),
      mitm_required_(mitm_required),
      feature_exchange_pending_(false),
      io_capability_(io_capability),
      bondable_mode_(bondable_mode),
      on_complete_(std::move(on_complete)),
      weak_ptr_factory_(this) {
  ZX_ASSERT(!(is_initiator() && peer_request_params_.has_value()));
  ZX_ASSERT(!(is_responder() && !peer_request_params_.has_value()));
  sm_chan().SetChannelHandler(weak_ptr_factory_.GetWeakPtr());
}

void Phase1::Start() {
  ZX_ASSERT(!has_failed());
  if (is_responder()) {
    ZX_ASSERT(peer_request_params_.has_value());
    RespondToPairingRequest(*peer_request_params_);
    return;
  }
  ZX_ASSERT(!peer_request_params_.has_value());
  InitiateFeatureExchange();
}

void Phase1::InitiateFeatureExchange() {
  // Only the initiator can initiate the feature exchange.
  ZX_ASSERT(is_initiator());
  // Pairing should not be in progress when this function is called
  ZX_ASSERT(!feature_exchange_pending_);
  auto pdu = util::NewPdu(sizeof(PairingRequestParams));
  if (!pdu) {
    bt_log(TRACE, "sm", "out of memory!");
    Abort(ErrorCode::kCommandNotSupported);
    return;
  }

  LocalPairingParams preq_values = BuildPairingParameters();
  PacketWriter writer(kPairingRequest, pdu.get());
  *writer.mutable_payload<PairingRequestParams>() =
      PairingRequestParams{.io_capability = preq_values.io_capability,
                           .oob_data_flag = preq_values.oob_data_flag,
                           .auth_req = preq_values.auth_req,
                           .max_encryption_key_size = preq_values.max_encryption_key_size,
                           .initiator_key_dist_gen = preq_values.local_keys,
                           .responder_key_dist_gen = preq_values.remote_keys};

  // Cache the pairing request. This will be passed out when Phase 1 completes.
  pdu->Copy(&pairing_payload_buffer_);
  feature_exchange_pending_ = true;
  sm_chan()->Send(std::move(pdu));
}

void Phase1::RespondToPairingRequest(const PairingRequestParams& req_params) {
  // We should only be in this state when pairing is initiated by the remote i.e. we are the slave.
  ZX_ASSERT(is_responder());
  ZX_ASSERT(!feature_exchange_pending_);
  feature_exchange_pending_ = true;

  auto pdu = util::NewPdu(sizeof(PairingResponseParams));
  PacketWriter writer(kPairingResponse, pdu.get());
  auto* rsp_params = writer.mutable_payload<PairingResponseParams>();

  LocalPairingParams pres_values = BuildPairingParameters();
  *rsp_params = PairingResponseParams{
      .io_capability = pres_values.io_capability,
      .oob_data_flag = pres_values.oob_data_flag,
      .auth_req = pres_values.auth_req,
      .max_encryption_key_size = pres_values.max_encryption_key_size,
  };
  // The keys that will be exchanged correspond to the intersection of what the
  // initiator requests and we support.
  rsp_params->initiator_key_dist_gen = pres_values.remote_keys & req_params.initiator_key_dist_gen;
  rsp_params->responder_key_dist_gen = pres_values.local_keys & req_params.responder_key_dist_gen;

  fit::result<PairingFeatures, ErrorCode> maybe_features =
      ResolveFeatures(false /* local_initiator */, req_params, *rsp_params);
  feature_exchange_pending_ = false;
  if (maybe_features.is_error()) {
    bt_log(TRACE, "sm", "rejecting pairing features");
    Abort(maybe_features.error());
    return;
  }
  PairingFeatures features = maybe_features.value();
  // If we've accepted a non-bondable pairing request in bondable mode as indicated by setting
  // features.will_bond false, we should reflect that in the rsp_params we send to the peer.
  if (!features.will_bond && bondable_mode_ == BondableMode::Bondable) {
    rsp_params->auth_req &= ~AuthReq::kBondingFlag;
  }
  // Copy the pairing response so that it's available after moving |pdu|. We want to ensure we
  // send the response before calling on_complete_, which can trigger other SMP transactions. The
  // copied |pres| value will be used for crypto functions in Phase 2.
  pdu->Copy(&pairing_payload_buffer_);
  sm_chan()->Send(std::move(pdu));

  StaticByteBuffer<util::PacketSize<PairingRequestParams>()> preq;
  PacketWriter preq_writer(kPairingRequest, &preq);
  auto* params = preq_writer.mutable_payload<PairingRequestParams>();
  *params = req_params;

  on_complete_(features, preq, pairing_payload_buffer_);
}

LocalPairingParams Phase1::BuildPairingParameters() {
  LocalPairingParams local_params;
  // If we are in non-bondable mode we will not distribute or request distribution of any bonding
  // data (i.e. there will be no key distribution phase) per V5.1 Vol 3 Part C Section 9.4.2.2
  if (bondable_mode_ == BondableMode::NonBondable) {
    local_params.auth_req = 0u;
    local_params.remote_keys = local_params.local_keys = 0;
  } else {
    local_params.auth_req = AuthReq::kBondingFlag;
    // We always request identity information from the remote.
    local_params.remote_keys = KeyDistGen::kIdKey;
    local_params.local_keys = 0;

    ZX_ASSERT(listener());
    if (listener()->OnIdentityRequest().has_value()) {
      local_params.local_keys |= KeyDistGen::kIdKey;
    }

    // When we are the master, we request that the peer send us encryption information as it is
    // required to do so (Vol 3, Part H, 2.4.2.3). Otherwise we always request to distribute it.
    // While Vol 3 Part H Section 3.6.1 says that the master may distribute their LTK as well, this
    // conditional check here ensures that only the responder will send their LTK. Our stack will
    // not handle both sides sending their LTK correctly, and one will be overwritten.
    if (is_initiator()) {
      local_params.remote_keys |= KeyDistGen::kEncKey;
    } else {
      local_params.local_keys |= KeyDistGen::kEncKey;
    }
  }
  if (sm_chan().SupportsSecureConnections()) {
    local_params.auth_req |= AuthReq::kSC;
  }
  if (mitm_required_) {
    local_params.auth_req |= AuthReq::kMITM;
  }

  local_params.io_capability = io_capability_;
  local_params.max_encryption_key_size = kMaxEncryptionKeySize;
  local_params.oob_data_flag = oob_available_ ? OOBDataFlag::kPresent : OOBDataFlag::kNotPresent;
  return local_params;
}

fit::result<PairingFeatures, ErrorCode> Phase1::ResolveFeatures(bool local_initiator,
                                                                const PairingRequestParams& preq,
                                                                const PairingResponseParams& pres) {
  ZX_ASSERT(feature_exchange_pending_);

  // Select the smaller of the initiator and responder max. encryption key size
  // values (Vol 3, Part H, 2.3.4).
  uint8_t enc_key_size = std::min(preq.max_encryption_key_size, pres.max_encryption_key_size);
  if (enc_key_size < kMinEncryptionKeySize) {
    bt_log(TRACE, "sm", "encryption key size too small! (%u)", enc_key_size);
    return fit::error(ErrorCode::kEncryptionKeySize);
  }

  bool will_bond = (preq.auth_req & kBondingFlag) && (pres.auth_req & kBondingFlag);
  if (!will_bond) {
    bt_log(INFO, "sm", "negotiated non-bondable pairing (local mode: %s)",
           bondable_mode_ == BondableMode::Bondable ? "bondable" : "non-bondable");
  }
  bool sc = (preq.auth_req & AuthReq::kSC) && (pres.auth_req & AuthReq::kSC);
  bool mitm = (preq.auth_req & AuthReq::kMITM) || (pres.auth_req & AuthReq::kMITM);
  bool init_oob = preq.oob_data_flag == OOBDataFlag::kPresent;
  bool rsp_oob = pres.oob_data_flag == OOBDataFlag::kPresent;

  IOCapability local_ioc, peer_ioc;
  if (local_initiator) {
    local_ioc = preq.io_capability;
    peer_ioc = pres.io_capability;
  } else {
    local_ioc = pres.io_capability;
    peer_ioc = preq.io_capability;
  }

  PairingMethod method =
      util::SelectPairingMethod(sc, init_oob, rsp_oob, mitm, local_ioc, peer_ioc, local_initiator);

  // If MITM protection is required but the pairing method cannot provide MITM,
  // then reject the pairing.
  if (mitm && method == PairingMethod::kJustWorks) {
    return fit::error(ErrorCode::kAuthenticationRequirements);
  }

  // The "Pairing Response" command (i.e. |pres|) determines the keys that shall
  // be distributed. The keys that will be distributed by us and the peer
  // depends on whichever one initiated the feature exchange by sending a
  // "Pairing Request" command.
  KeyDistGenField local_keys, remote_keys;
  if (local_initiator) {
    local_keys = pres.initiator_key_dist_gen;
    remote_keys = pres.responder_key_dist_gen;

    // v5.1, Vol 3, Part H Section 3.6.1 requires that the responder shall not set to one
    // any flag in the key dist gen fields that the initiator has set to zero.
    // Hence we reject the pairing if the responder requests keys that we don't
    // support.
    if ((preq.initiator_key_dist_gen & local_keys) != local_keys ||
        (preq.responder_key_dist_gen & remote_keys) != remote_keys) {
      return fit::error(ErrorCode::kInvalidParameters);
    }
  } else {
    local_keys = pres.responder_key_dist_gen;
    remote_keys = pres.initiator_key_dist_gen;

    // When we are the responder we always respect the initiator's wishes.
    ZX_ASSERT((preq.initiator_key_dist_gen & remote_keys) == remote_keys);
    ZX_ASSERT((preq.responder_key_dist_gen & local_keys) == local_keys);
  }
  // v5.1 Vol 3 Part C Section 9.4.2.2 says that bonding information shall not be exchanged or
  // stored in non-bondable mode. This check ensures that we avoid a situation where, if we were in
  // bondable mode and a peer requested non-bondable mode with a non-zero keydistgen field, we pair
  // in non-bondable mode but also attempt to distribute keys.
  if (!will_bond && (local_keys || remote_keys)) {
    return fit::error(ErrorCode::kInvalidParameters);
  }

  return fit::ok(PairingFeatures(local_initiator, sc, will_bond, method, enc_key_size, local_keys,
                                 remote_keys));
}

void Phase1::OnPairingResponse(const PairingResponseParams& response_params) {
  // Support receiving a pairing response only as the initiator.
  if (is_responder()) {
    bt_log(TRACE, "sm", "received pairing response when acting as responder");
    Abort(ErrorCode::kCommandNotSupported);
    return;
  }

  if (!feature_exchange_pending_) {
    bt_log(TRACE, "sm", "ignoring unexpected \"Pairing Response\" packet");
    return;
  }

  fit::result<PairingFeatures, ErrorCode> maybe_features = ResolveFeatures(
      true /* local_initiator */,
      pairing_payload_buffer_.view(sizeof(Code)).As<PairingRequestParams>(), response_params);
  feature_exchange_pending_ = false;
  if (maybe_features.is_error()) {
    bt_log(TRACE, "sm", "rejecting pairing features");
    Abort(maybe_features.error());
    return;
  }
  PairingFeatures features = maybe_features.value();

  StaticByteBuffer<util::PacketSize<PairingRequestParams>()> pres_buffer;
  auto writer = PacketWriter(kPairingResponse, &pres_buffer);
  *writer.mutable_payload<PairingResponseParams>() = response_params;
  on_complete_(features, pairing_payload_buffer_, pres_buffer);
}

void Phase1::OnRxBFrame(ByteBufferPtr sdu) {
  fit::result<ValidPacketReader, ErrorCode> maybe_reader = ValidPacketReader::ParseSdu(sdu);
  if (maybe_reader.is_error()) {
    Abort(maybe_reader.error());
    return;
  }
  ValidPacketReader reader = maybe_reader.value();
  Code smp_code = reader.code();

  if (smp_code == kPairingFailed) {
    OnFailure(Status(reader.payload<ErrorCode>()));
  } else if (smp_code == kPairingResponse) {
    OnPairingResponse(reader.payload<PairingResponseParams>());
  } else {
    bt_log(INFO, "sm", "received unexpected code %#.2X when in Pairing Phase 1", smp_code);
    Abort(ErrorCode::kUnspecifiedReason);
  }
}

}  // namespace sm
}  // namespace bt

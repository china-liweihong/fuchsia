// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_CORE_PTYSVC_PTY_TRANSACTION_H_
#define ZIRCON_SYSTEM_CORE_PTYSVC_PTY_TRANSACTION_H_

#include <lib/fidl/llcpp/transaction.h>

// This adapter is necessary for going between the llcpp-style Transaction and
// the C binding style fidl_txn_t.
class PtyTransaction : public fidl::Transaction {
 public:
  explicit PtyTransaction(fidl_txn_t* txn) : txn_(txn) {}

  ~PtyTransaction() override {
    ZX_ASSERT_MSG(status_called_,
                  "Transaction must have it's Status() method used.  This provides "
                  "HandleFsSpecificMessage with the correct status value.\n");
  }

  /// Status() return the internal state of the DDK transaction. This MUST be called
  /// to bridge the Transaction and DDK dispatcher.
  zx_status_t Status() __WARN_UNUSED_RESULT {
    status_called_ = true;
    return status_;
  }

 protected:
  void Reply(fidl::Message msg) final {
    ZX_ASSERT(txn_);

    const fidl_msg_t fidl_msg{
        .bytes = msg.bytes().data(),
        .handles = msg.handles().data(),
        .num_bytes = static_cast<uint32_t>(msg.bytes().size()),
        .num_handles = static_cast<uint32_t>(msg.handles().size()),
    };

    status_ = txn_->reply(txn_, &fidl_msg);
  }

  void Close(zx_status_t close_status) final { status_ = close_status; }

  std::unique_ptr<fidl::Transaction> TakeOwnership() final {
    ZX_ASSERT_MSG(false, "PTY Transaction cannot take ownership of the transaction.\n");
  }

 private:
  fidl_txn_t* txn_;
  zx_status_t status_ = ZX_OK;
  bool status_called_ = false;
};

#endif  // ZIRCON_SYSTEM_CORE_PTYSVC_PTY_TRANSACTION_H_

// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UI_A11Y_LIB_SCREEN_READER_FOCUS_TESTS_MOCKS_MOCK_A11Y_FOCUS_MANAGER_H_
#define SRC_UI_A11Y_LIB_SCREEN_READER_FOCUS_TESTS_MOCKS_MOCK_A11Y_FOCUS_MANAGER_H_

#include "src/ui/a11y/lib/focus_chain/accessibility_focus_chain_listener.h"
#include "src/ui/a11y/lib/screen_reader/focus/a11y_focus_manager.h"

namespace accessibility_test {
class MockA11yFocusManager : public a11y::A11yFocusManager {
 public:
  MockA11yFocusManager();
  ~MockA11yFocusManager() override = default;

  // Returns the current a11y focus.
  std::optional<a11y::A11yFocusManager::A11yFocusInfo> GetA11yFocus() override;

  // Function for setting a11y focus.
  void SetA11yFocus(zx_koid_t koid, uint32_t node_id,
                    a11y::A11yFocusManager::SetA11yFocusCallback callback) override;

  // Returns true if IsGetA11yFocusCalled was called.
  bool IsGetA11yFocusCalled() const;

  // Returns true if IsSetA11yFocusCalled was called.
  bool IsSetA11yFocusCalled() const;

 private:
  // Tracks if GetA11yFocus() is called.
  bool get_a11y_focus_called_ = false;

  // Tracks if SetA11yFocus() is called.
  bool set_a11y_focus_called_ = true;

  // Stores the A11yFocusInfo which was sent in SetA11yFocusInfo.
  a11y::A11yFocusManager::A11yFocusInfo a11y_focus_info_;
};
}  // namespace accessibility_test

#endif  // SRC_UI_A11Y_LIB_SCREEN_READER_FOCUS_TESTS_MOCKS_MOCK_A11Y_FOCUS_MANAGER_H_
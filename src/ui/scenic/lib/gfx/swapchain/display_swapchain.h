// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UI_SCENIC_LIB_GFX_SWAPCHAIN_DISPLAY_SWAPCHAIN_H_
#define SRC_UI_SCENIC_LIB_GFX_SWAPCHAIN_DISPLAY_SWAPCHAIN_H_

#include <fuchsia/sysmem/cpp/fidl.h>
#include <lib/zx/event.h>
#include <lib/zx/handle.h>
#include <lib/zx/vmo.h>

#include "src/lib/fxl/memory/weak_ptr.h"
#include "src/ui/lib/escher/flib/fence_listener.h"
#include "src/ui/lib/escher/resources/resource_manager.h"
#include "src/ui/lib/escher/resources/resource_recycler.h"
#include "src/ui/lib/escher/vk/vulkan_device_queues.h"
#include "src/ui/scenic/lib/gfx/swapchain/swapchain.h"

#include <vulkan/vulkan.hpp>

namespace scenic_impl {
namespace gfx {

class Display;
class DisplayManager;
class Sysmem;

namespace test {
class DisplaySwapchainTest;
}

// DisplaySwapchain implements the Swapchain interface by using a Vulkan
// swapchain to present images to a physical display using the Zircon
// display controller API.
class DisplaySwapchain : public Swapchain {
 public:
  DisplaySwapchain(Sysmem* sysmem, DisplayManager* display_manager, Display* display,
                   escher::Escher* escher);
  ~DisplaySwapchain() override;

  // Callback to call on every vsync. Arguments are:
  // - the timestamp of the vsync.
  using OnVsyncCallback = fit::function<void(zx::time vsync_timestamp)>;

  // |Swapchain|
  bool DrawAndPresentFrame(const FrameTimingsPtr& frame_timings, size_t swapchain_index,
                           const HardwareLayerAssignment& hla, zx::event frame_retired,
                           DrawCallback draw_callback) override;

  // Register a callback to be called on each vsync.
  // Only allows a single listener at a time.
  void RegisterVsyncListener(OnVsyncCallback on_vsync) {
    FXL_CHECK(!on_vsync_);
    on_vsync_ = std::move(on_vsync);
  }

  // Remove the registered vsync listener.
  void UnregisterVsyncListener() { on_vsync_ = nullptr; }

  // Passes along color correction information to the display
  void SetDisplayColorConversion(const ColorTransform& transform) override;

  // Set the state for protected memory usage in |use_protected_memory_|. If there is a state
  // change to true, it reallocates |swapchain_buffers_| using protected memory.
  void SetUseProtectedMemory(bool use_protected_memory) override;

 private:
  friend class test::DisplaySwapchainTest;

  struct Framebuffer {
    zx::vmo vmo;
    escher::GpuMemPtr device_memory;
    escher::ImagePtr escher_image;
    uint64_t fb_id;
  };

  struct FrameRecord {
    // The "pending" frame is the one that has not yet been rendered.
    FrameTimingsPtr pending_frame_timings;
    size_t pending_swapchain_index;
    // This timing is for the frame that is rendered or retired.
    FrameTimingsPtr frame_timings;
    size_t swapchain_index;

    escher::SemaphorePtr render_finished_escher_semaphore;
    uint64_t render_finished_event_id;
    zx::event render_finished_event;
    std::unique_ptr<async::Wait> render_finished_wait;

    // Event is signaled when the display is done using a frame.
    std::unique_ptr<async::Wait> frame_retired_wait;
    uint64_t frame_retired_event_id;

    // Event is signaled when the display is done using a buffer.
    escher::SemaphorePtr retired_escher_semaphore;
    uint64_t retired_event_id;
    zx::event retired_event;

    bool presented = false;
  };
  std::unique_ptr<FrameRecord> NewFrameRecord();

  bool InitializeFramebuffers(escher::ResourceRecycler* resource_recycler,
                              bool use_protected_memory);
  bool CreateBuffer(fuchsia::sysmem::BufferCollectionTokenSyncPtr local_token,
                    escher::ResourceRecycler* resource_recycler, bool use_protected_memory,
                    Framebuffer* buffer);

  // When a frame is presented, the previously-presented frame becomes available
  // as a render target.
  void OnFrameRendered(size_t frame_index, zx::time render_finished_time);

  void OnVsync(zx::time timestamp, const std::vector<uint64_t>& image_ids);

  // A nullable Escher (good for testing) means some resources must be accessed
  // through its (valid) pointer.
  escher::Escher* const escher_ = nullptr;

  Sysmem* sysmem_;
  DisplayManager* display_manager_;
  Display* const display_;

  size_t next_frame_index_ = 0;
  size_t presented_frame_idx_ = 0;
  size_t outstanding_frame_count_ = 0;
  bool use_protected_memory_ = false;

  std::vector<Framebuffer> swapchain_buffers_;
  // Optionally generated on the fly.
  std::vector<Framebuffer> protected_swapchain_buffers_;

  std::vector<std::unique_ptr<FrameRecord>> frames_;

  vk::Format format_;
  vk::Device device_;
  vk::Queue queue_;

  OnVsyncCallback on_vsync_;

  FXL_DISALLOW_COPY_AND_ASSIGN(DisplaySwapchain);
};

}  // namespace gfx
}  // namespace scenic_impl

#endif  // SRC_UI_SCENIC_LIB_GFX_SWAPCHAIN_DISPLAY_SWAPCHAIN_H_

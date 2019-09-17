// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_CORE_DEVMGR_DEVCOORDINATOR_SYSTEM_INSTANCE_H_
#define ZIRCON_SYSTEM_CORE_DEVMGR_DEVCOORDINATOR_SYSTEM_INSTANCE_H_

#include <fuchsia/boot/c/fidl.h>
#include <fuchsia/ldsvc/llcpp/fidl.h>
#include <lib/zx/vmo.h>

#include "boot-args.h"
#include "coordinator.h"

constexpr char kItemsPath[] = "/svc/" fuchsia_boot_Items_Name;

zx_status_t wait_for_file(const char* path, zx::time deadline);
zx_status_t get_ramdisk(zx::vmo* ramdisk_vmo);

class SystemInstance {
 public:
  SystemInstance();

  // TODO(ZX-4860): DEPRECATED. Do not add new dependencies on the fshost loader service!
  zx_status_t clone_fshost_ldsvc(zx::channel* loader);

  void do_autorun(const char* name, const char* cmd);

  struct ServiceStarterArgs {
    SystemInstance* instance;
    devmgr::Coordinator* coordinator;
  };

  // Thread entry point
  static int fuchsia_starter(void* arg);

  int FuchsiaStarter(devmgr::Coordinator* coordinator);

  static int service_starter(void* arg);

  int ServiceStarter(devmgr::Coordinator* coordinator);

  struct ConsoleStarterArgs {
    SystemInstance* instance;
    devmgr::BootArgs* boot_args;
  };

  static int console_starter(void* arg);

  int ConsoleStarter(const devmgr::BootArgs* arg);

  // Thread entry point
  static int pwrbtn_monitor_starter(void* arg);

  int PwrbtnMonitorStarter();

  void start_console_shell(const devmgr::BootArgs& boot_args);

  zx_status_t CreateFuchsiaJob(const zx::job& root_job);

  zx_status_t StartSvchost(const zx::job& root_job, bool require_system,
                           devmgr::Coordinator* coordinator, zx::channel fshost_client);

  void fshost_start(devmgr::Coordinator* coordinator, const devmgr::DevmgrArgs& devmgr_args,
                    zx::channel fshost_server);

  void devmgr_vfs_init(devmgr::Coordinator* coordinator, const devmgr::DevmgrArgs& devmgr_args,
                       zx::channel fshost_server);

  // The handle used to transmit messages to appmgr.
  zx::channel appmgr_client;

  // The handle used by appmgr to serve incoming requests.
  // If appmgr cannot be launched within a timeout, this handle is closed.
  zx::channel appmgr_server;

  // The handle used to transmit messages to miscsvc.
  zx::channel miscsvc_client;

  // The handle used by miscsvc to serve incoming requests.
  zx::channel miscsvc_server;

  // The handle used to transmit messages to device_name_provider.
  zx::channel device_name_provider_client;

  // The handle used by device_name_provider to serve incoming requests.
  zx::channel device_name_provider_server;

  // Handle to the loader service hosted in fshost, which allows loading from /boot and /system
  // rather than specific packages.
  // This isn't actually "optional", it's just initialized later.
  // TODO(ZX-4860): Delete this once all dependencies have been removed.
  fit::optional<llcpp::fuchsia::ldsvc::Loader::SyncClient> fshost_ldsvc;

  zx::job svc_job;
  zx::job fuchsia_job;
  zx::channel svchost_outgoing;

  zx::channel fs_root;

  // Used to bind the svchost to the virtual-console binary to provide fidl
  // services.
  zx::channel virtcon_fidl;
};

#endif  // ZIRCON_SYSTEM_CORE_DEVMGR_DEVCOORDINATOR_SYSTEM_INSTANCE_H_

// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <threads.h>

#include <zircon/compiler.h>
#include <zircon/thread_annotations.h>
#include <zircon/types.h>
#include <fdio/io.h>
#include <fdio/remoteio.h>
#include <fdio/vfs.h>

#ifdef __cplusplus

#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fbl/atomic.h>
#include <fbl/intrusive_double_list.h>
#include <fbl/ref_ptr.h>
#include <fbl/unique_ptr.h>
#include <fs/remote.h>
#include <fs/watcher.h>

#include "dnode.h"

namespace memfs {

class Dnode;

class VnodeMemfs : public fs::Vnode {
public:
    virtual zx_status_t Setattr(const vnattr_t* a) final;
    virtual zx_status_t Sync() final;
    zx_status_t Ioctl(uint32_t op, const void* in_buf, size_t in_len,
                      void* out_buf, size_t out_len, size_t* out_actual) override;
    zx_status_t AttachRemote(fs::MountChannel h) final;

    // To be more specific: Is this vnode connected into the directory hierarchy?
    // VnodeDirs can be unlinked, and this method will subsequently return false.
    bool IsDirectory() const { return dnode_ != nullptr; }
    void UpdateModified() { modify_time_ = zx_time_get(ZX_CLOCK_UTC); }

    virtual ~VnodeMemfs();

    fbl::RefPtr<Dnode> dnode_;
    uint32_t link_count_;

protected:
    VnodeMemfs();

    uint64_t ino_;
    uint64_t create_time_;
    uint64_t modify_time_;

private:
    static fbl::atomic<uint64_t> ino_ctr_;
};

class VnodeFile final : public VnodeMemfs {
public:
    VnodeFile();
    VnodeFile(zx_handle_t vmo, zx_off_t length);
    ~VnodeFile();

    virtual zx_status_t Open(uint32_t flags, fbl::RefPtr<Vnode>* out_redirect) final;

private:
    zx_status_t Read(void* data, size_t len, size_t off, size_t* out_actual) final;
    zx_status_t Write(const void* data, size_t len, size_t off, size_t* out_actual) final;
    zx_status_t Truncate(size_t len) final;
    zx_status_t Getattr(vnattr_t* a) final;
    zx_status_t Mmap(int flags, size_t len, size_t* off, zx_handle_t* out) final;

    zx_handle_t vmo_;
    zx_off_t length_;
};

class VnodeDir : public VnodeMemfs {
public:
    VnodeDir();
    virtual ~VnodeDir();

    virtual zx_status_t Open(uint32_t flags, fbl::RefPtr<Vnode>* out_redirect) final;
    zx_status_t Lookup(fbl::RefPtr<fs::Vnode>* out, const char* name, size_t len) final;
    zx_status_t Create(fbl::RefPtr<fs::Vnode>* out, const char* name, size_t len, uint32_t mode) final;

    // Create a vnode from a VMO.
    // Fails if the vnode already exists.
    // Passes the vmo to the Vnode; does not duplicate it.
    zx_status_t CreateFromVmo(bool vmofile, const char* name, size_t namelen, zx_handle_t vmo,
                              zx_off_t off, zx_off_t len);

    // Mount a subtree as a child of this directory.
    void MountSubtree(fbl::RefPtr<VnodeDir> subtree);

    // Use the watcher container to implement a directory watcher
    void Notify(const char* name, size_t len, unsigned event) final;
    zx_status_t WatchDir(fs::Vfs* vfs, const vfs_watch_dir_t* cmd) final;

    // The vnode is acting as a mount point for a remote filesystem or device.
    virtual bool IsRemote() const final;
    virtual zx::channel DetachRemote() final;
    virtual zx_handle_t GetRemote() const final;
    virtual void SetRemote(zx::channel remote) final;

private:
    zx_status_t Readdir(fs::vdircookie_t* cookie, void* dirents, size_t len) final;

    // Resolves the question, "Can this directory create a child node with the name?"
    // Returns "ZX_OK" on success; otherwise explains failure with error message.
    zx_status_t CanCreate(const char* name, size_t namelen) const;

    // Creates a dnode for the Vnode, attaches vnode to dnode, (if directory) attaches
    // dnode to vnode, and adds dnode to parent directory.
    zx_status_t AttachVnode(fbl::RefPtr<memfs::VnodeMemfs> vn, const char* name, size_t namelen,
                            bool isdir);

    zx_status_t Unlink(const char* name, size_t len, bool must_be_dir) final;
    zx_status_t Rename(fbl::RefPtr<fs::Vnode> newdir,
                       const char* oldname, size_t oldlen,
                       const char* newname, size_t newlen,
                       bool src_must_be_dir, bool dst_must_be_dir) final;
    zx_status_t Link(const char* name, size_t len, fbl::RefPtr<fs::Vnode> target) final;
    zx_status_t Getattr(vnattr_t* a) final;
    zx_status_t Mmap(int flags, size_t len, size_t* off, zx_handle_t* out) final;
    zx_status_t Ioctl(uint32_t op, const void* in_buf, size_t in_len,
                      void* out_buf, size_t out_len, size_t* out_actual) final;

    fs::RemoteContainer remoter_;
    fs::WatcherContainer watcher_;
};

class VnodeVmo final : public VnodeMemfs {
public:
    VnodeVmo(zx_handle_t vmo, zx_off_t offset, zx_off_t length);
    ~VnodeVmo();

    virtual zx_status_t Open(uint32_t flags, fbl::RefPtr<Vnode>* out_redirect) override;

private:
    zx_status_t Serve(fs::Vfs* vfs, zx::channel channel, uint32_t flags) final;
    zx_status_t Read(void* data, size_t len, size_t off, size_t* out_actual) final;
    zx_status_t Getattr(vnattr_t* a) final;
    zx_status_t GetHandles(uint32_t flags, zx_handle_t* hnds,
                           uint32_t* type, void* extra, uint32_t* esize) final;

    zx_handle_t vmo_;
    zx_off_t offset_;
    zx_off_t length_;
    bool have_local_clone_;
};

} // namespace memfs

using VnodeMemfs = memfs::VnodeMemfs;
using VnodeDir = memfs::VnodeDir;

fbl::RefPtr<VnodeDir> BootfsRoot();
fbl::RefPtr<VnodeDir> MemfsRoot();
fbl::RefPtr<VnodeDir> SystemfsRoot();
fbl::RefPtr<VnodeDir> DevfsRoot();

#else

typedef struct VnodeMemfs VnodeMemfs;
typedef struct VnodeDir VnodeDir;

#endif  // ifdef __cplusplus

__BEGIN_CDECLS

void vfs_global_init(VnodeDir* root);

// generate mxremoteio handles
zx_handle_t vfs_create_global_root_handle(void);
zx_status_t vfs_connect_global_root_handle(zx_handle_t h);
zx_handle_t vfs_create_root_handle(VnodeMemfs* vn);
zx_status_t vfs_connect_root_handle(VnodeMemfs* vn, zx_handle_t h);

// device fs
zx_status_t devfs_mount(zx_handle_t h);

// boot fs
zx_status_t bootfs_add_file(const char* path, zx_handle_t vmo, zx_off_t off, size_t len);

// system fs
VnodeDir* systemfs_get_root(void);
zx_status_t systemfs_add_file(const char* path, zx_handle_t vmo, zx_off_t off, size_t len);

// Create the global root to memfs
VnodeDir* vfs_create_global_root(void) TA_NO_THREAD_SAFETY_ANALYSIS;

// Create a generic root to memfs
VnodeDir* vfs_create_root(void);

void memfs_mount(VnodeDir* parent, VnodeDir* subtree);

void devmgr_vfs_exit(void);

__END_CDECLS

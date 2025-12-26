#include "HelloFS.hpp"
#include <memory>
#include <iostream>

// 全域指標：指向目前運作的 FS 實例
static std::unique_ptr<FileSystem> fs_instance;

// 橋接函式 (Trampolines)
namespace bridge {
    int getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
        return fs_instance->GetAttr(path, stbuf);
    }
    int unlink(const char *path) {
        return fs_instance->Unlink(path);
    }
    int open(const char *path, struct fuse_file_info *fi) {
        return fs_instance->Open(path, fi);
    }
    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
        return fs_instance->Read(path, buf, size, offset, fi);
    }
    int write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
        return fs_instance->Write(path, buf, size, offset, fi);
    }
    int release(const char *path, struct fuse_file_info *fi) {
        return fs_instance->Release(path, fi);
    }
    int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
        return fs_instance->ReadDir(path, buf, filler, offset, fi, flags);
    }
    int create(const char *path, mode_t mode, struct fuse_file_info *fi) {
        return fs_instance->Create(path, mode, fi);
    }
}

// FUSE Operations Table
static const struct fuse_operations myfs_oper = {
    .getattr = bridge::getattr,
    .unlink  = bridge::unlink,
    // truncate
    .open    = bridge::open,
    .read    = bridge::read,
    .write   = bridge::write,
    .release = bridge::release,
    .readdir = bridge::readdir,
    .create  = bridge::create,
};

int main(int argc, char *argv[]) {
    fs_instance = std::make_unique<HelloFS>();

    umask(0);
    return fuse_main(argc, argv, &myfs_oper, NULL);
}
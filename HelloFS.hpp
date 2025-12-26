#pragma once

#include "FileSystem.hpp"
#include <string_view>
#include <map>

class HelloFS : public FileSystem {
public:
    HelloFS(); // 建構子

    // 必須實作的介面 (Override)
    int GetAttr(const char *path, struct stat *stbuf) override;
    int Unlink(const char *path) override;
    int Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) override;
    int Open(const char *path, struct fuse_file_info *fi) override;
    int Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) override;
    int Release(const char *path, struct fuse_file_info *fi) override;
    int ReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) override;
    int Create(const char *path, mode_t mode, struct fuse_file_info *fi) override;
};
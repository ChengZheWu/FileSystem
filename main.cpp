#include "HelloFS.hpp"
#include <memory>
#include <iostream>
#include <unistd.h>
#include <climits> // for PATH_MAX

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
    // 1. 取得目前的絕對路徑
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string storagePath = std::string(cwd) + "/storage";
        
        std::cout << "Mounting HelloFS with backing store: " << storagePath << std::endl;
        
        // 2. 初始化 FS，傳入路徑
        fs_instance = std::make_unique<HelloFS>(storagePath);
    } else {
        perror("getcwd() error");
        return 1;
    }

    umask(0);
    return fuse_main(argc, argv, &myfs_oper, NULL);
}
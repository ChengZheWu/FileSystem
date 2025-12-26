#define FUSE_USE_VERSION 31
#pragma once
#include <fuse.h>
#include <sys/stat.h>
#include <string>

// Interface
class FileSystem {
public:
    // 使用虛擬解構子
    // 否則用 Base 指標 delete Derived 物件時會 Memory Leak。
    virtual ~FileSystem() = default;

    // 定義純虛擬函式 (Pure Virtual Functions)
    // "= 0" 代表子類別「必須」實作這些功能
    virtual int GetAttr(const char *path, struct stat *stbuf) = 0;
    virtual int Unlink(const char *path) = 0;
    virtual int Open(const char *path, struct fuse_file_info *fi) = 0;
    virtual int Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) = 0;
    virtual int Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) = 0;
    virtual int Release(const char *path, struct fuse_file_info *fi) = 0;
    virtual int ReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) = 0;
    virtual int Create(const char *path, mode_t mode, struct fuse_file_info *fi) = 0;

    
    // 初始化 (Optional)
    virtual void Init() {}
};
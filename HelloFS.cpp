#include "HelloFS.hpp"
#include <cstring>
#include <cerrno>
#include <iostream>
#include <algorithm> // std::min, std::copy

// 使用匿名 Namespace 來存放常數，類似 static 的效果，避免汙染全域
namespace {
    using namespace std::literals;
    constexpr auto FILE_NAME    = "hello"sv;
    constexpr auto FILE_PATH    = "/hello"sv;
    constexpr auto FILE_CONTENT = "Hello World! This is OOP FUSE.\n"sv;
}

HelloFS::HelloFS() {
    std::cout << "[HelloFS] Initialized" << std::endl;
}

int HelloFS::GetAttr(const char *path, struct stat *stbuf) {
    std::memset(stbuf, 0, sizeof(struct stat));
    
    // 根目錄
    if (path == "/"sv) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    } 
    
    // 我們的虛擬檔案
    if (path == FILE_PATH) {
        stbuf->st_mode = S_IFREG | 0444; // 0444 = Read Only
        stbuf->st_nlink = 1;
        stbuf->st_size = FILE_CONTENT.size();
        return 0;
    }

    return -ENOENT;
}

int HelloFS::Unlink(const char *path) {
    (void) path;
    return -EROFS;
}

int HelloFS::Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    if (path != FILE_PATH)
        return -ENOENT;

    size_t len = FILE_CONTENT.size();
    if (static_cast<size_t>(offset) >= len)
        return 0;

    if (offset + size > len)
        size = len - offset;

    std::copy(FILE_CONTENT.begin() + offset,
              FILE_CONTENT.begin() + offset + size,
              buf);

    return size;
}

int HelloFS::Open(const char *path, struct fuse_file_info *fi) {
    if (path != FILE_PATH)
        return -ENOENT;

    // 檢查有沒有人試圖用 Write 模式開啟 (雖然 GetAttr 給了唯讀，但還是防一下)
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    return 0;
}

// --- 以下是不支援的操作，直接回傳錯誤 ---

int HelloFS::Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) path; (void) buf; (void) size; (void) offset; (void) fi;
    return -EROFS; // Read-Only File System
}

int HelloFS::Release(const char *path, struct fuse_file_info *fi) {
    (void) path; (void) fi;
    return 0; // 沒做什麼特別的資源分配，直接 return 0
}

int HelloFS::ReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;

    if (path != "/"sv)
        return -ENOENT;

    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, FILE_NAME.data(), nullptr, 0, FUSE_FILL_DIR_PLUS);

    return 0;
}

int HelloFS::Create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) path; (void) mode; (void) fi;
    return -EROFS;
}
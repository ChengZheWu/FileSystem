#pragma once

#include "FileSystem.hpp"
#include "FSLayout.hpp" // 記得 include 剛剛寫好的 Layout
#include <unistd.h>
#include <fcntl.h>

class HelloFS : public FileSystem {
private:
    int m_fd;           // myfs.img 的檔案描述符
    Superblock m_sb;    // 記憶體中緩存的 Superblock

    // Helper: 給定 inode index，把資料讀出來
    // 回傳 true 代表讀取成功
    bool GetInode(uint32_t inode_no, Inode* out_inode);

    // Helper: 給定路徑 (e.g., "/test"), 找出對應的 Inode
    // 回傳 true 代表找到
    bool LookupInode(const char* path, Inode* out_inode);

    // 讀取 Bitmap Block，找到第一個是 0 (空閒) 的 bit，回傳 index
    // start_block: Bitmap 存放在第幾個 Block (Inode=1, Data=2)
    // max_count: 總共有幾個 bit (Inode=128, Data=1024)
    int AllocateResource(int bitmap_block_idx, int max_count);

    // 把指定 index 的 bit 設為 1 (佔用) 或 0 (釋放) 並寫回硬碟
    void SetResourceStatus(int bitmap_block_idx, int index, bool used);
public:
    HelloFS(const char* img_path);
    ~HelloFS();

    // 必須實作的介面 (Override)
    int GetAttr(const char *path, struct stat *stbuf) override;
    int Unlink(const char *path) override;
    int Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) override;
    int Open(const char *path, struct fuse_file_info *fi) override;
    int Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) override;
    int Release(const char *path, struct fuse_file_info *fi) override;
    int ReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) override;
    int Create(const char *path, mode_t mode, struct fuse_file_info *fi) override;
    int Utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) override;
};
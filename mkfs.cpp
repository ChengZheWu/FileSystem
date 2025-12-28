#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include "FSLayout.hpp"

// 簡單的錯誤檢查
void error_exit(const char* msg) {
    perror(msg);
    exit(1);
}

int main() {
    const char* filename = "myfs.img";
    std::cout << "[mkfs] Creating " << filename << " with size " << DISK_SIZE << " bytes..." << std::endl;

    // 1. 開檔 (Create, Truncate, Read/Write)
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd == -1) error_exit("open");

    // 2. 設定檔案大小為 10MB (ftruncate 會自動用 0 填滿)
    if (ftruncate(fd, DISK_SIZE) == -1) error_exit("ftruncate");

    // --- 寫入 Superblock (Block 0) ---
    Superblock sb;
    sb.magic = MYFS_MAGIC;
    sb.block_size = BLOCK_SIZE;
    sb.inode_count = NUM_INODES;
    sb.data_block_count = NUM_DATA_BLOCKS;
    
    // 寫入 Superblock
    if (write(fd, &sb, sizeof(Superblock)) != sizeof(Superblock)) 
        error_exit("write superblock");
    
    // 初始化 Inode Bitmap (Block 1)
    // 我們需要標記 Inode 0 為「已使用」，因為 Inode 0 通常保留給根目錄 "/"
    // 這裡我們先簡單跳過 Bitmap 的複雜操作，直接去初始化 Inode Table
    // 在真實 Driver 中，你會在這裡把 Block 1 的第一個 bit 設為 1

    // 我們必須標記 Inode 0 為「已使用」，否則 Create 會誤用 Inode 0 覆蓋根目錄
    uint8_t inode_bitmap[BLOCK_SIZE];
    std::memset(inode_bitmap, 0, BLOCK_SIZE);
    // 設定第 0 個 byte 的第 0 個 bit 為 1 (代表 Inode 0 已佔用)
    inode_bitmap[0] = 1;
    // 寫入 Block 1 (Offset = 1 * BLOCK_SIZE)
    // pwrite(int fd, const void *buf, size_t count, off_t offset)
    if (pwrite(fd, inode_bitmap, BLOCK_SIZE, 1 * BLOCK_SIZE) != BLOCK_SIZE) {
        error_exit("write inode bitmap");
    }

    // 初始化 Data Bitmap (Block 2)
    // 雖然預設可能就是 0，但我們明確寫入全 0，確保乾淨
    uint8_t data_bitmap[BLOCK_SIZE];
    std::memset(data_bitmap, 0, BLOCK_SIZE); 
    // 目前沒有任何 Data Block 被使用，所以全 0
    if (pwrite(fd, data_bitmap, BLOCK_SIZE, 2 * BLOCK_SIZE) != BLOCK_SIZE) {
        error_exit("write data bitmap");
    }
    
    // 初始化 Inode Table (從 Block 3 開始)
    // 建立 Root Directory (Inode 0)
    Inode root_inode;
    std::memset(&root_inode, 0, sizeof(root_inode));
    root_inode.inode_no = 0;
    root_inode.type = FT_DIR; // 它是個資料夾
    root_inode.size = 0;      // 資料夾目前是空的
    root_inode.block_no = 0;  // 暫時不指向任何 Data Block
    std::strcpy(root_inode.filename, "/");

    // 計算 Block 3 的 offset
    off_t inode_table_offset = 3 * BLOCK_SIZE;
    // 寫入 Root Inode
    if (pwrite(fd, &root_inode, sizeof(Inode), inode_table_offset) != sizeof(Inode)) {
        error_exit("write root inode");
    }

    std::cout << "[mkfs] Formatting done. Root inode created (Inode 0 reserved)." << std::endl;

    close(fd);
    return 0;
}
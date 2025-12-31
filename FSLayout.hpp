#pragma once
#include <cstdint>

// 硬碟設定
const uint64_t BLOCK_SIZE = 4096;       // 4KB
const uint64_t DISK_SIZE  = 10 * 1024 * 1024; // 10MB
const uint64_t NUM_INODES = 128;        // 只支援 128 個檔案
// Data Blocks 數量 = (總大小 / Block大小) - Metadata 所佔的空間
// 先簡單抓個大概，保留前 10 個 Block 給 Metadata
const uint64_t NUM_DATA_BLOCKS = (DISK_SIZE / BLOCK_SIZE) - 10; // 2560 - 10 = 2550
const uint64_t DATA_BLOCK_START = 10; // Data Block 從第 10 號 Block 開始

// Magic Number: 用來識別這是不是我們的 FS
const uint32_t MYFS_MAGIC = 0xDEADBEEF; 

// 1. Superblock: 硬碟的第 0 個 Block
struct Superblock {
    uint32_t magic;           // 0xDEADBEEF
    uint32_t block_size;      // 4096
    uint32_t inode_count;     // 128
    uint32_t data_block_count;// 可用的資料區塊數
    
    // Padding: 補滿 4096 bytes (雖然不補也不影響功能，但這是好習慣)
    uint8_t padding[BLOCK_SIZE - 4 * 4]; 
};

// 檔案類型
enum FileType : uint8_t {
    FT_UNKNOWN = 0,
    FT_REG_FILE = 1, // 一般檔案
    FT_DIR = 2       // 資料夾
};

// 2. Inode: 每個檔案的身分證
struct Inode {
    uint32_t inode_no;      // 4 bytes  編號 (0 ~ 127)
    uint32_t size;          // 4 bytes  檔案大小 (Bytes)

    // 先假設一個檔案「最多只能佔用一個 Block (4KB)」
    // 這樣就先不需要實作複雜的 Block List 或 Indirection
    uint32_t block_no;      // 4 bytes  指向 Data Region 的第幾個 Block
    uint8_t  type;          // 1 byte   FileType

    // 手動填補 Padding，確保結構緊湊且對齊
    // 目前用了: 4+4+4+1 = 13 bytes
    uint8_t  padding[3];    // 補 3 bytes，讓後面湊齊到 16 bytes 邊界
    
    char     filename[32];  // 32 bytes  檔名 (固定 32 chars)
    
    // 總共目前：13 + 3 + 32 = 48 bytes
    // 我們把它湊滿 64 bytes，剩下的保留給未來擴充 (Reserved)
    uint8_t  reserved[16];  // 64 - 48 = 16 bytes
};

// 計算 Inode Table 需要幾個 Block
// (sizeof(Inode) * NUM_INODES) / BLOCK_SIZE ...
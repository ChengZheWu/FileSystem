#include "HelloFS.hpp"
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fcntl.h>      // open
#include <unistd.h>     // pread, pwrite, close, unlink
#include <sys/stat.h>   // lstat
#include <dirent.h>     // opendir, readdir

HelloFS::HelloFS(const char* img_path) {
    std::cout << "[HelloFS] Mounting image: " << img_path << std::endl;
    
    // 1. 打開 myfs.img
    m_fd = open(img_path, O_RDWR);
    if (m_fd == -1) {
        perror("open image failed");
        exit(1);
    }

    // 2. 讀取 Superblock (位於 Block 0)
    // pread(fd, buf, size, offset)
    if (pread(m_fd, &m_sb, sizeof(Superblock), 0) != sizeof(Superblock)) {
        std::cerr << "Failed to read superblock" << std::endl;
        exit(1);
    }

    // 3. 驗證 Magic Number
    if (m_sb.magic != MYFS_MAGIC) {
        std::cerr << "Invalid Magic Number! Are you sure this is formatted by mkfs?" << std::endl;
        exit(1);
    }
    
    std::cout << "[HelloFS] Mount Success! Inodes: " << m_sb.inode_count << std::endl;
}

HelloFS::~HelloFS() {
    if (m_fd != -1) close(m_fd);
}

// 讀取第 N 個 Inode 的資料
bool HelloFS::GetInode(uint32_t inode_no, Inode* out_inode) {
    if (inode_no >= m_sb.inode_count) return false;

    // Inode Table 從 Block 3 開始 (Block 0=SB, Block 1=IBitmap, Block 2=DBitmap)
    // 這裡為了簡單，我們寫死 Offset。正規做法應該計算 block_size * 3 + index * size
    off_t offset = (3 * BLOCK_SIZE) + (inode_no * sizeof(Inode));
    
    if (pread(m_fd, out_inode, sizeof(Inode), offset) != sizeof(Inode)) {
        return false;
    }
    return true;
}

// 暴力搜尋：遍歷所有 Inode 找檔名
bool HelloFS::LookupInode(const char* path, Inode* out_inode) {
    // FUSE 傳進來的 path 都是絕對路徑，例如 "/" 或 "/hello"

    // 處理傳入的路徑：如果開頭是 "/"，就跳過它
    const char* name_to_search = path;
    
    // 只有在「不是根目錄」的情況下，才去掉開頭的 '/'
    // 如果 path 是 "/"，我們就保留 "/"，這樣才能對應到 mkfs 存進去的檔名
    std::string s_path = path;
    if (s_path != "/") {
        if (name_to_search[0] == '/') {
            name_to_search++;
        }
    }
    
    // 遍歷整個 Inode Table
    for (uint32_t i = 0; i < m_sb.inode_count; ++i) {
        Inode node;
        if (!GetInode(i, &node)) continue;

        // 如果該 Inode 是空的 (Type=0)，就跳過
        if (node.type == 0) continue;

        // 比對檔名，使用處理過的 name_to_search 進行比對
        // 注意：我們的 mkfs 把 Root 命名為 "/"
        if (std::strcmp(node.filename, name_to_search) == 0) {
            *out_inode = node;
            return true;
        }
        
        // 額外處理：如果 path 是 "/abc"，但我們存的是 "abc" (有些設計會去掉 /)
        // 但我們的 mkfs 範例是直接存 "/"，所以先單純比對即可
    }
    return false;
}

// 尋找空閒資源 (First Fit 演算法)
int HelloFS::AllocateResource(int bitmap_block_idx, int max_count) {
    // 進入此函式馬上上鎖！
    // 直到這個函式 return 之前，其他執行緒都會被擋在外面等待
    std::lock_guard<std::mutex> lock(m_alloc_mutex);

    // 讀取整個 Bitmap Block
    uint8_t bitmap[BLOCK_SIZE];
    off_t offset = bitmap_block_idx * BLOCK_SIZE;
    if (pread(m_fd, bitmap, BLOCK_SIZE, offset) != BLOCK_SIZE) {
        return -1;
    }

    for (int i = 0; i < max_count; ++i) {
        // 計算這個 bit 在第幾個 byte 的第幾個 bit
        int byte_idx = i / 8;
        int bit_idx  = i % 8;

        // 檢查該 bit 是否為 0
        if (!((bitmap[byte_idx] >> bit_idx) & 0x1)) {
            // 找到空位了！馬上標記為已使用
            bitmap[byte_idx] |= (1 << bit_idx);
            pwrite(m_fd, bitmap, BLOCK_SIZE, offset);
            return i; // 回傳編號
        }
    }
    return -1; // 沒空間了 (鎖會在這裡自動釋放)
}

// 更新 Bitmap
void HelloFS::SetResourceStatus(int bitmap_block_idx, int index, bool used) {
    // 釋放資源或強制設定時，也要上鎖
    std::lock_guard<std::mutex> lock(m_alloc_mutex);

    uint8_t bitmap[BLOCK_SIZE];
    off_t offset = bitmap_block_idx * BLOCK_SIZE;

    // 先讀出來 (Read-Modify-Write)
    pread(m_fd, bitmap, BLOCK_SIZE, offset);

    int byte_idx = index / 8;
    int bit_idx  = index % 8;

    if (used) {
        bitmap[byte_idx] |= (1 << bit_idx);  // 設為 1
    } else {
        bitmap[byte_idx] &= ~(1 << bit_idx); // 設為 0
    }

    // 寫回去
    pwrite(m_fd, bitmap, BLOCK_SIZE, offset);
}

int HelloFS::GetAttr(const char *path, struct stat *stbuf) {
    std::memset(stbuf, 0, sizeof(struct stat));
    
    Inode inode;
    if (!LookupInode(path, &inode)) {
        return -ENOENT;
    }

    // 將我們自定義的 Inode 轉成 Linux 的 stat
    if (inode.type == FT_DIR) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (inode.type == FT_REG_FILE) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
    }
    
    stbuf->st_size = inode.size;
    return 0;
}

int HelloFS::Unlink(const char *path) {
    return -errno;
}

int HelloFS::Open(const char *path, struct fuse_file_info *fi) {
    return -errno;
}

int HelloFS::Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    Inode inode;
    if (!LookupInode(path, &inode)) {
        return -ENOENT;
    }

    // 如果 offset 超過檔案大小，讀不到東西
    if (offset >= inode.size) return 0;
    
    // 如果讀取長度超過檔案剩餘大小，截斷它
    if (offset + size > inode.size) {
        size = inode.size - offset;
    }

    // 如果檔案是空的，或者還沒分配 Block
    if (inode.block_no == 0) return 0;

    // 計算資料位置
    off_t data_offset = inode.block_no * BLOCK_SIZE + offset;
    
    if (pread(m_fd, buf, size, data_offset) != (ssize_t)size) {
        return -EIO;
    }

    return size;
}

int HelloFS::Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    
    // 1. 讀取 Inode
    Inode inode;
    if (!LookupInode(path, &inode)) {
        return -ENOENT;
    }

    // 我們只支援 4KB 以下的小檔案
    // 限制：寫入的「終點位置 (offset + size)」不能超過 Block 的大小
    if (size > BLOCK_SIZE) {
        return -EFBIG; // File too large
    }

    // 2. 檢查是否已經分配了 Data Block
    if (inode.block_no == 0) {
        // 還沒分配，去要一塊 (Block 2 是 Data Bitmap)
        int block_idx = AllocateResource(2, m_sb.data_block_count);
        if (block_idx == -1) {
            return -ENOSPC;
        }
        
        // 我們的 Data Area 從 Block 10 開始 (前面留給 Superblock/Bitmap/Inodes)
        // 這裡要跟 FSLayout 的定義對齊。假設前 10 個 Block 是 Metadata。
        // 所以 Data Block 0 的真實位置是 Block 10
        inode.block_no = DATA_BLOCK_START + block_idx;
        
        std::cout << "[Write] Allocated Data Block " << inode.block_no << " (Index " << block_idx << ")" << std::endl;
    }

    // 3. 寫入資料到 Data Block
    off_t data_offset = inode.block_no * BLOCK_SIZE + offset;
    if (pwrite(m_fd, buf, size, data_offset) != (ssize_t)size) {
        return -EIO;
    }

    // 4. 更新 Inode (大小)
    // 如果新寫入的範圍超過原本大小，才更新 size
    if (offset + size > inode.size) {
        inode.size = offset + size;
    }

    // 5. 把更新後的 Inode 寫回去
    off_t inode_offset = (3 * BLOCK_SIZE) + (inode.inode_no * sizeof(Inode));
    pwrite(m_fd, &inode, sizeof(Inode), inode_offset);

    return size;
}

int HelloFS::Release(const char *path, struct fuse_file_info *fi) {
    return -errno;
}

int HelloFS::ReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;

    // 只有根目錄 "/" 可以列出檔案
    if (std::string(path) != "/") return -ENOENT;

    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);

    // 掃描 Inode Table，把所有存在的檔案都列出來
    // 因為我們是扁平化結構，所有檔案都在根目錄下
    for (uint32_t i = 1; i < m_sb.inode_count; ++i) { // 從 1 開始，跳過 0 (Root)
        Inode node;
        if (GetInode(i, &node)) {
            if (node.type != 0) {
                // node.filename 可能是 "/file"，顯示時要把前面的 "/" 去掉，不然 ls 會怪怪的
                const char* name = node.filename;
                if (name[0] == '/') name++; 
                
                filler(buf, name, nullptr, 0, FUSE_FILL_DIR_PLUS);
            }
        }
    }
    return 0;
}

int HelloFS::Create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode; (void) fi;
    std::cout << "[Create] Creating file: " << path << std::endl;

    // 1. 找一個空的 Inode (Block 1 是 Inode Bitmap)
    int inode_idx = AllocateResource(1, m_sb.inode_count);
    if (inode_idx == -1) {
        return -ENOSPC; // No space left on device
    }

    // 2. 準備 Inode 資料
    Inode new_inode;
    std::memset(&new_inode, 0, sizeof(Inode)); // 清空
    new_inode.inode_no = inode_idx;
    new_inode.type = FT_REG_FILE; // 一般檔案
    new_inode.size = 0;           // 剛建立大小是 0
    new_inode.block_no = 0;       // 還沒寫資料，所以沒有 Data Block
    
    // 處理檔名 (要把路徑前面的 "/" 去掉，只存檔名)
    // path = "/abc", 我們存 "abc"
    const char* filename = path;
    if (filename[0] == '/') filename++;
    std::strncpy(new_inode.filename, filename, 31);

    // 3. 寫入 Inode Table
    off_t offset = (3 * BLOCK_SIZE) + (inode_idx * sizeof(Inode));
    if (pwrite(m_fd, &new_inode, sizeof(Inode), offset) != sizeof(Inode)) {
        return -EIO;
    }

    std::cout << "[Create] Success! Inode " << inode_idx << " allocated." << std::endl;
    return 0;
}

int HelloFS::Utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    (void) path; (void) tv; (void) fi;
    return 0; // 假裝時間設定成功
}
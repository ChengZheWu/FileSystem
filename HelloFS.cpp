#include "HelloFS.hpp"
#include <cstring>
#include <cerrno>
#include <iostream>
#include <algorithm> // std::min, std::copy

HelloFS::HelloFS() {
    std::cout << "[HelloFS] Initialized" << std::endl;
    // 初始化預設檔案
    m_files["/hello"] = "Hello World! This is OOP FUSE.\n";
}

int HelloFS::GetAttr(const char *path, struct stat *stbuf) {
    std::memset(stbuf, 0, sizeof(struct stat));
    
    if (std::string(path) == "/") {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    } 

    // 檢查檔案是否存在於我們的 map 中
    auto it = m_files.find(path);
    if (it != m_files.end()) {
        // 找到了！
        stbuf->st_mode = S_IFREG | 0666; // 0666 = Read/Write
        stbuf->st_nlink = 1;
        stbuf->st_size = it->second.size(); // 真實的檔案大小
        return 0;
    }

    return -ENOENT;
}

int HelloFS::Unlink(const char *path) {
    auto count = m_files.erase(path);
    return (count > 0) ? 0 : -ENOENT;
}

int HelloFS::Open(const char *path, struct fuse_file_info *fi) {
    if (m_files.find(path) == m_files.end())
        return -ENOENT;
    return 0;
}

int HelloFS::Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    
    auto it = m_files.find(path);
    if (it == m_files.end())
        return -ENOENT;

    const std::string &content = it->second;
    size_t len = content.size();

    if (static_cast<size_t>(offset) >= len)
        return 0;

    if (offset + size > len)
        size = len - offset;

    memcpy(buf, content.data() + offset, size);

    return size;
}

// --- 以下是不支援的操作，直接回傳錯誤 ---

int HelloFS::Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    std::cout << "[Write] path: " << path << ", size: " << size << ", offset: " << offset << std::endl;

    auto it = m_files.find(path);
    if (it == m_files.end()) {
        return -ENOENT;
    }

    std::string &content = it->second;

    // 如果寫入位置超過目前檔案大小，需要擴充檔案 (用 '\0' 填補中間空洞)
    if (offset + size > content.size()) {
        content.resize(offset + size, '\0');
    }

    // 將資料 copy 到 string 的指定位置
    // copy(來源起始, 來源結束, 目標起始)
    // 這裡我們直接用 std::string 的 replace 或 copy 也可以，但 memcpy/loop 最直觀
    for (size_t i = 0; i < size; ++i) {
        content[offset + i] = buf[i];
    }

    return size; // 回傳實際寫入的 byte 數
}

int HelloFS::Release(const char *path, struct fuse_file_info *fi) {
    (void) path; (void) fi;
    return 0; // 沒做什麼特別的資源分配，直接 return 0
}

int HelloFS::ReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;

    if (std::string(path) != "/")
        return -ENOENT;

    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);

    // 遍歷我們的 map，把所有檔案名稱都列出來
    for (const auto& [filepath, content] : m_files) {
        // map 裡存的是 "/hello", "/test"，但 readdir 只需要檔名 "hello", "test"
        // 我們簡單處理：把開頭的 "/" 去掉
        if (filepath.size() > 1 && filepath[0] == '/') {
            filler(buf, filepath.c_str() + 1, nullptr, 0, FUSE_FILL_DIR_PLUS);
        }
    }

    return 0;
}

int HelloFS::Create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode; (void) fi;
    std::cout << "[Create] " << path << std::endl;
    
    if (m_files.count(path)) {
        return -EEXIST; // 檔案已存在
    }

    // 建立一個空的檔案內容
    m_files[path] = ""; 
    return 0;
}
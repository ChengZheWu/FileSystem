#include "HelloFS.hpp"
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fcntl.h>      // open
#include <unistd.h>     // pread, pwrite, close, unlink
#include <sys/stat.h>   // lstat
#include <dirent.h>     // opendir, readdir

HelloFS::HelloFS(std::string rootPath) : m_root_path(std::move(rootPath)) {
    // 確保路徑最後沒有 '/'，方便後面拼接
    if (!m_root_path.empty() && m_root_path.back() == '/') {
        m_root_path.pop_back();
    }
    std::cout << "[HelloFS] Storage Root: " << m_root_path << std::endl;
}

std::string HelloFS::GetRealPath(const char* path) {
    return m_root_path + path;
}

int HelloFS::GetAttr(const char *path, struct stat *stbuf) {
    std::string realPath = GetRealPath(path);
    
    // lstat 是 Linux System Call，直接讀取真實檔案的屬性
    int res = lstat(realPath.c_str(), stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

int HelloFS::Unlink(const char *path) {
    std::string realPath = GetRealPath(path);
    int res = unlink(realPath.c_str());
    if (res == -1)
        return -errno;
    return 0;
}

int HelloFS::Open(const char *path, struct fuse_file_info *fi) {
    std::string realPath = GetRealPath(path);
    
    // 將 FUSE 的 flags 直接傳給底層 open
    int fd = open(realPath.c_str(), fi->flags);
    if (fd == -1)
        return -errno;

    // ★ 關鍵：把 fd 存在 fuse_file_info 裡，傳給 Read/Write 使用
    fi->fh = static_cast<uint64_t>(fd);
    
    return 0;
}

int HelloFS::Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) path;
    // 直接用 Open 時存下來的 fd (fi->fh)
    // pread 會從指定的 offset 讀取，不會改變檔案的當前指標，適合多執行緒
    int res = pread(fi->fh, buf, size, offset);
    if (res == -1)
        return -errno;

    return res;
}

int HelloFS::Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) path;
    // pwrite 直接寫入指定 offset
    int res = pwrite(fi->fh, buf, size, offset);
    if (res == -1)
        return -errno;

    return res;
}

int HelloFS::Release(const char *path, struct fuse_file_info *fi) {
    (void) path;
    // 關閉檔案描述符
    close(fi->fh);
    return 0;
}

int HelloFS::ReadDir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    std::string realPath = GetRealPath(path);
    DIR *dp = opendir(realPath.c_str());
    if (dp == NULL)
        return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        std::memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        
        // 將真實目錄下的檔案回報給 FUSE
        if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS))
            break;
    }

    closedir(dp);
    return 0;
}

int HelloFS::Create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    std::string realPath = GetRealPath(path);

    // 建立新檔案
    int fd = open(realPath.c_str(), fi->flags, mode);
    if (fd == -1)
        return -errno;

    fi->fh = static_cast<uint64_t>(fd);
    return 0;
}
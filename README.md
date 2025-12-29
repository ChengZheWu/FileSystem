# HelloFS: A Simple User-Space File System (FUSE)

HelloFS is a simple file system developed based on **FUSE** (Filesystem in Userspace) technology. It simulates the design architecture of a classic Unix file system, implementing core concepts including the Superblock, Inode, Bitmap allocation, and data block management.

The main purpose of this project is to provide an understanding of file system operations at the operating system level. It includes a complete formatting tool (`mkfs`) and a mountable driver (`myfs`).

## 📋 Table of Contents

1. [Environment Setup](https://www.google.com/search?q=%23-environment-setup)
2. [Compilation & Execution](https://www.google.com/search?q=%23-compilation--execution)
3. [Implementation Highlights](https://www.google.com/search?q=%23-implementation-highlights)
4. [Pros & Cons](https://www.google.com/search?q=%23%25EF%25B8%258F-pros--cons)
5. [Testing Process](https://www.google.com/search?q=%23-testing-process)
6. [Future Roadmap](https://www.google.com/search?q=%23-future-roadmap)

---

## 🛠 Environment Setup

This project is developed using the **C++17** standard and depends on `libfuse3`. It is recommended to run this in a Linux environment (e.g., Ubuntu or Debian).

### Install Dependencies

```bash
sudo apt update
sudo apt install build-essential fuse3 libfuse3-dev pkg-config

```

### Configure FUSE Permissions

Ensure that `user_allow_other` is enabled (uncommented) in `/etc/fuse.conf` so that non-root users can mount the file system normally.

---

## 🚀 Compilation & Execution

The project provides a `Makefile` to automatically compile the driver and the formatting tool.

### 1. Compile

```bash
make
# Outputs:
# - myfs (FUSE Driver)
# - mkfs (Formatting Tool)

```

### 2. Format

Before mounting, the disk image (`myfs.img`) must be initialized and the basic structures (Superblock, Root Inode) must be created.

```bash
./mkfs
# Output: [mkfs] Formatting done. Root inode created (Inode 0 reserved).

```

### 3. Mount

Create a mount point and start the FUSE Driver. It is recommended to use the `-f` flag to run in the foreground and observe logs.

```bash
mkdir -p mnt
./myfs -f mnt

```

### 4. Unmount

When testing is finished, use the following command to safely unmount:

```bash
fusermount3 -u mnt

```

---

## 💡 Implementation Highlights

HelloFS adopts a classic design similar to EXT2 and implements complete resource management logic in User Space.

### 1. Disk Layout

We use a large file (`myfs.img`) to simulate a Block Device. The Block Size is **4096 Bytes**.

| Block Index | Region Name | Description |
| --- | --- | --- |
| **0** | Superblock | Stores global info (Magic Number, Block Count, Inode Count) |
| **1** | Inode Bitmap | Records Inode usage status (0=Free, 1=Used) |
| **2** | Data Bitmap | Records Data Block usage status |
| **3 ~ N** | Inode Table | Stores `struct Inode` array (Metadata, Size, Filename) |
| **10 ~ End** | Data Region | Area where actual file content is stored |

### 2. Core Functions

* **Persistence**: Uses `pread`/`pwrite` to perform direct I/O on the image file. Data remains persistent across reboots.
* **Resource Allocation**: Implements a Bitmap-based lookup algorithm (`AllocateResource`) to automatically find and mark free Inodes and Data Blocks.
* **Overwrite & Truncate**: Supports the `O_TRUNC` flag and `truncate` operations, solving the "Ghost Tail" (residual data) issue.
* **File Deletion (Unlink)**: Implements a full resource recovery mechanism. Deleting a file automatically frees the corresponding Bitmap bits for reuse by subsequent files.

### 3. Concurrency Control

To support the multi-threaded nature of FUSE, the **Monitor Pattern** is used for lock management:

* **Public Interface** (`Create`, `Write`, `Unlink`...): Responsible for acquiring the `std::mutex` lock.
* **Private Helpers** (`AllocateResource`, `LookupInode`...): Do not handle locking; they focus on logic execution to avoid Deadlocks.
* **Atomic Operations**: Ensures that reading, modifying, and writing back Bitmaps is an atomic operation to prevent Race Conditions.

---

## ⚖️ Pros & Cons

### ✅ Pros

1. **Clear Structure**: Adopts classic Unix FS architecture, making it an excellent example for learning file system design.
2. **Thread-Safe**: Maintains data consistency under high concurrent write loads through fine-grained Mutex design.
3. **Random R/W Support**: Implements Offset-based `pwrite`, supporting `lseek` and random position modifications.
4. **Complete Lifecycle**: Covers the full process from Formatting (`mkfs`) to Creation, Writing, Reading, and Deletion.
5. **Data Structure Alignment**: The Inode design occupies 48 Bytes but is padded to 64 Bytes to prevent Inodes from crossing Block boundaries, improving efficiency.

### ⚠️ Cons & Limitations

1. **File Size Limit (Single Block)**: Currently, each file can only correspond to **one Data Block** (4KB). Writing more than 4KB returns an `-EFBIG` error.
2. **Flat Directory Structure**: Does not support subdirectories; all files reside under the root `/`.
3. **Performance Bottlenecks**:
* Uses User Space `read/write` interfaces, involving multiple Kernel/User memory copies (Non-Zero-Copy).
* Coarse-grained locking may impact performance under extremely high loads.


4. **No Journaling**: Power loss during a write operation may lead to inconsistencies between Bitmap and Inode states.

---

## 🧪 Testing Process (How to Test)

After mounting successfully, open a new terminal window to perform the following tests:

### 1. Create & Write

```bash
echo "Hello World" > mnt/a.txt
cat mnt/a.txt
# Expected Output: Hello World
# Log should show: Inode allocated, Data Block allocated

```

### 2. Overwrite Test (Verify Truncate)

```bash
echo "Hi" > mnt/a.txt
cat mnt/a.txt
# Expected Output: Hi (Should not see residual data like "Hillo World")

```

### 3. Random Write (Patching)

```bash
# Write XXX into the middle of Hello World
echo "1234567890" > mnt/b.txt
echo -n "XXX" | dd of=mnt/b.txt bs=1 seek=3 conv=notrunc
cat mnt/b.txt
# Expected Output: 123XXX7890

```

### 4. Deletion & Resource Recovery Verification

```bash
rm mnt/a.txt
touch mnt/c.txt
# Observe Log: c.txt should reuse the Inode number freed by a.txt

```

---

## 🔮 Future Roadmap

1. **Multi-block Support**:
* Modify Inode structure to add `block_no` arrays or Indirect Pointers to support files larger than 4KB.


2. **Directory Support**:
* Implement directory Inode types, allowing Data Blocks to store Directory Entries instead of raw text.


3. **Performance Optimization (Zero-Copy)**:
* Migrate underlying I/O from `pread/pwrite` to the `splice` system call to enable direct data transfer within the Kernel.


4. **Enter Kernel Space**:
* Port core logic to a **Linux Kernel Module (.ko)** and register it as a Character Device to experience real driver development.
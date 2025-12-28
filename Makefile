# Compiler settings
CXX = g++
# 通用的編譯旗標 (包含 C++17 和 FUSE 的 Header路徑)
CXXFLAGS = -Wall -std=c++17 $(shell pkg-config fuse3 --cflags)

# FUSE 的 Library (連結時才需要，只有 myfs 需要)
FUSE_LIBS = $(shell pkg-config fuse3 --libs)

# 目標檔案名稱
TARGET_FS = myfs
TARGET_TOOL = mkfs

# Source files
# FS 需要 main.cpp 和 HelloFS.cpp
SRCS_FS = main.cpp HelloFS.cpp
OBJS_FS = $(SRCS_FS:.cpp=.o)

# mkfs 只需要 mkfs.cpp
SRCS_TOOL = mkfs.cpp
OBJS_TOOL = $(SRCS_TOOL:.cpp=.o)

# Default rule: 打 'make' 會同時編譯兩個
all: $(TARGET_FS) $(TARGET_TOOL)

# --- 1. 編譯 myfs (File System) ---
$(TARGET_FS): $(OBJS_FS)
	$(CXX) -o $@ $^ $(FUSE_LIBS)

# --- 2. 編譯 mkfs (Format Tool) ---
# mkfs 不需要連結 FUSE library，只要一般的 C++ 編譯即可
$(TARGET_TOOL): $(OBJS_TOOL)
	$(CXX) -o $@ $^

# 通用的 .cpp -> .o 編譯規則
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean: 清除所有產生的檔案
clean:
	rm -f *.o $(TARGET_FS) $(TARGET_TOOL) myfs.img

# Phony targets
.PHONY: all clean
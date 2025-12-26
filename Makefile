# Compiler settings
CXX = g++
CXXFLAGS = -Wall -std=c++17 $(shell pkg-config fuse3 --cflags)
LDFLAGS = $(shell pkg-config fuse3 --libs)

# Target executable name
TARGET = myfs

# Source files
SRCS = main.cpp HelloFS.cpp
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compile
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
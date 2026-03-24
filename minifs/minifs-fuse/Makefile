# MiniFS Makefile

# Compiler
CC = gcc

# Détection automatique de FUSE
FUSE_CFLAGS := $(shell pkg-config --cflags fuse3 2>/dev/null || pkg-config --cflags fuse 2>/dev/null || echo "-I/usr/include/fuse")
FUSE_LIBS := $(shell pkg-config --libs fuse3 2>/dev/null || pkg-config --libs fuse 2>/dev/null || echo "-lfuse")

# Flags de compilation
CFLAGS = -Wall -Wextra -std=c11 -D_FILE_OFFSET_BITS=64 $(FUSE_CFLAGS)
LDFLAGS = $(FUSE_LIBS) -lpthread

# Flags debug/release
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG

# Répertoires
SRC_DIR = src
INCLUDE_DIR = include
TOOLS_DIR = tools
BUILD_DIR = build
BIN_DIR = bin

# Installation
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SBINDIR = $(PREFIX)/sbin

# Sources
MAIN_SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/filesystem.c $(SRC_DIR)/inode.c \
            $(SRC_DIR)/block.c $(SRC_DIR)/directory.c $(SRC_DIR)/file_ops.c \
            $(SRC_DIR)/utils.c $(SRC_DIR)/debug.c

MKFS_SRCS = $(TOOLS_DIR)/mkfs.c $(SRC_DIR)/filesystem.c $(SRC_DIR)/inode.c \
            $(SRC_DIR)/block.c $(SRC_DIR)/utils.c

FSCK_SRCS = $(TOOLS_DIR)/fsck.c $(SRC_DIR)/filesystem.c $(SRC_DIR)/inode.c \
            $(SRC_DIR)/block.c $(SRC_DIR)/utils.c $(SRC_DIR)/debug.c

DEBUG_SRCS = $(TOOLS_DIR)/debugfs.c $(SRC_DIR)/filesystem.c $(SRC_DIR)/inode.c \
             $(SRC_DIR)/block.c $(SRC_DIR)/utils.c $(SRC_DIR)/debug.c

MOUNT_SRCS = $(TOOLS_DIR)/mount_helper.c

# Objets
MAIN_OBJS = $(MAIN_SRCS:%.c=$(BUILD_DIR)/%.o)
MKFS_OBJS = $(MKFS_SRCS:%.c=$(BUILD_DIR)/%.o)
FSCK_OBJS = $(FSCK_SRCS:%.c=$(BUILD_DIR)/%.o)
DEBUG_OBJS = $(DEBUG_SRCS:%.c=$(BUILD_DIR)/%.o)
MOUNT_OBJS = $(MOUNT_SRCS:%.c=$(BUILD_DIR)/%.o)

# Exécutables
MAIN_EXEC = $(BIN_DIR)/minifs_fuse
MKFS_EXEC = $(BIN_DIR)/mkfs.minifs
FSCK_EXEC = $(BIN_DIR)/fsck.minifs
DEBUG_EXEC = $(BIN_DIR)/debugfs.minifs
MOUNT_EXEC = $(BIN_DIR)/mount.minifs

.PHONY: all clean install uninstall debug release test help

all: release

$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

$(BUILD_DIR)/$(SRC_DIR) $(BUILD_DIR)/$(TOOLS_DIR): | $(BUILD_DIR)
	@mkdir -p $@

$(BUILD_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/$(SRC_DIR)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) $(if $(DEBUG),$(DEBUG_FLAGS),$(RELEASE_FLAGS)) \
		-I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/$(TOOLS_DIR)/%.o: $(TOOLS_DIR)/%.c | $(BUILD_DIR)/$(TOOLS_DIR)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) $(if $(DEBUG),$(DEBUG_FLAGS),$(RELEASE_FLAGS)) \
		-I$(INCLUDE_DIR) -c $< -o $@

$(MAIN_EXEC): $(MAIN_OBJS) | $(BIN_DIR)
	@echo "  LD      $@"
	@$(CC) $^ $(LDFLAGS) -o $@

$(MKFS_EXEC): $(MKFS_OBJS) | $(BIN_DIR)
	@echo "  LD      $@"
	@$(CC) $^ -lpthread -o $@

$(FSCK_EXEC): $(FSCK_OBJS) | $(BIN_DIR)
	@echo "  LD      $@"
	@$(CC) $^ -lpthread -o $@

$(DEBUG_EXEC): $(DEBUG_OBJS) | $(BIN_DIR)
	@echo "  LD      $@"
	@$(CC) $^ -lpthread -o $@

$(MOUNT_EXEC): $(MOUNT_OBJS) | $(BIN_DIR)
	@echo "  LD      $@"
	@$(CC) $^ -o $@

release: $(MAIN_EXEC) $(MKFS_EXEC) $(FSCK_EXEC) $(DEBUG_EXEC) $(MOUNT_EXEC)
	@echo "Build complete (release mode)"

debug: DEBUG=1
debug: clean release
	@echo "Build complete (debug mode)"

install: release
	@echo "Installing MiniFS..."
	@install -d $(DESTDIR)$(BINDIR)
	@install -d $(DESTDIR)$(SBINDIR)
	@install -m 755 $(MAIN_EXEC) $(DESTDIR)$(BINDIR)/
	@install -m 755 $(MKFS_EXEC) $(DESTDIR)$(SBINDIR)/
	@install -m 755 $(FSCK_EXEC) $(DESTDIR)$(SBINDIR)/
	@install -m 755 $(DEBUG_EXEC) $(DESTDIR)$(SBINDIR)/
	@install -m 755 $(MOUNT_EXEC) $(DESTDIR)$(SBINDIR)/
	@echo "Installation complete"

uninstall:
	@echo "Uninstalling MiniFS..."
	@rm -f $(DESTDIR)$(BINDIR)/minifs_fuse
	@rm -f $(DESTDIR)$(SBINDIR)/mkfs.minifs
	@rm -f $(DESTDIR)$(SBINDIR)/fsck.minifs
	@rm -f $(DESTDIR)$(SBINDIR)/debugfs.minifs
	@rm -f $(DESTDIR)$(SBINDIR)/mount.minifs
	@echo "Uninstallation complete"

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

test: release
	@echo "Running test suite..."
	@./tests/test_suite.sh

help:
	@echo "MiniFS Build System"
	@echo ""
	@echo "Configuration détectée:"
	@echo "  FUSE_CFLAGS: $(FUSE_CFLAGS)"
	@echo "  FUSE_LIBS: $(FUSE_LIBS)"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build (default: release)"
	@echo "  release     - Build optimized"
	@echo "  debug       - Build with debug"
	@echo "  install     - Install (requires root)"
	@echo "  uninstall   - Uninstall (requires root)"
	@echo "  clean       - Clean build"
	@echo "  test        - Run tests"
	@echo "  help        - Show this help"

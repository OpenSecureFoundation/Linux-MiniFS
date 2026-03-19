# Makefile — MiniFS
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -I src

SRC     = src
BIN     = bin

MKFS    = $(BIN)/mkfs_mini
FUSE    = $(BIN)/minifs_fuse
TEST    = $(BIN)/test_core

.PHONY: all clean test test_core

all: $(BIN) $(MKFS) $(FUSE)

$(BIN):
	mkdir -p $(BIN)

# --- mkfs_mini ---
$(MKFS): $(SRC)/mkfs_mini.c $(SRC)/minifs.h | $(BIN)
	$(CC) $(CFLAGS) -o $@ $(SRC)/mkfs_mini.c

# --- minifs_fuse (necessite fuse3-devel) ---
$(FUSE): $(SRC)/minifs_fuse.c $(SRC)/minifs_core.c $(SRC)/minifs_core.h $(SRC)/minifs.h | $(BIN)
	$(CC) $(CFLAGS) $$(pkg-config --cflags fuse3) \
	      -o $@ $(SRC)/minifs_fuse.c $(SRC)/minifs_core.c \
	      $$(pkg-config --libs fuse3)

# --- test unitaire core (sans FUSE) ---
$(TEST): $(SRC)/test_core.c $(SRC)/minifs_core.c $(SRC)/minifs_core.h $(SRC)/minifs.h | $(BIN)
	$(CC) $(CFLAGS) -o $@ $(SRC)/test_core.c $(SRC)/minifs_core.c

test: $(MKFS) $(TEST)
	@echo "=== Test mkfs_mini ==="
	./$(MKFS) minifs.img 10
	python3 $(SRC)/verify_sb.py minifs.img
	@echo ""
	@echo "=== Test unitaire couche core ==="
	./$(TEST) minifs.img

test_core: $(TEST)
	./$(TEST) minifs.img

clean:
	rm -rf $(BIN) minifs.img

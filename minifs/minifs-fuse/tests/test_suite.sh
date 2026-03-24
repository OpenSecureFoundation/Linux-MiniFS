#!/bin/bash
# MiniFS Test Suite

set -e

TEST_IMG="test_minifs.img"
TEST_MOUNT="/tmp/minifs_test"
PASSED=0
FAILED=0

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

cleanup() {
    sudo umount "$TEST_MOUNT" 2>/dev/null || true
    rm -f "$TEST_IMG"
    rm -rf "$TEST_MOUNT"
}

pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((PASSED++))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((FAILED++))
}

echo "=== MiniFS Test Suite ==="
echo ""

# Cleanup before starting
cleanup

# Test 1: Create disk image
echo "Test 1: Creating disk image..."
dd if=/dev/zero of="$TEST_IMG" bs=1M count=10 &>/dev/null
if [ -f "$TEST_IMG" ]; then
    pass "Disk image created"
else
    fail "Failed to create disk image"
    exit 1
fi

# Test 2: Format filesystem
echo "Test 2: Formatting filesystem..."
if mkfs.minifs "$TEST_IMG" --force &>/dev/null; then
    pass "Filesystem formatted"
else
    fail "Failed to format filesystem"
    cleanup
    exit 1
fi

# Test 3: Mount filesystem
echo "Test 3: Mounting filesystem..."
mkdir -p "$TEST_MOUNT"
if sudo mount.minifs "$TEST_IMG" "$TEST_MOUNT" -o default_permissions &>/dev/null; then
    sleep 1
    if mountpoint -q "$TEST_MOUNT"; then
        pass "Filesystem mounted"
    else
        fail "Mount command succeeded but filesystem not mounted"
    fi
else
    fail "Failed to mount filesystem"
    cleanup
    exit 1
fi

# Test 4: Create file
echo "Test 4: Creating file..."
if echo "Hello MiniFS!" > "$TEST_MOUNT/test.txt" 2>/dev/null; then
    pass "File created"
else
    fail "Failed to create file"
fi

# Test 5: Read file
echo "Test 5: Reading file..."
CONTENT=$(cat "$TEST_MOUNT/test.txt" 2>/dev/null || echo "")
if [ "$CONTENT" == "Hello MiniFS!" ]; then
    pass "File read successfully"
else
    fail "File content mismatch (expected 'Hello MiniFS!', got '$CONTENT')"
fi

# Test 6: List directory
echo "Test 6: Listing directory..."
if ls "$TEST_MOUNT" | grep -q "test.txt"; then
    pass "Directory listing works"
else
    fail "File not visible in directory listing"
fi

# Test 7: Create multiple files
echo "Test 7: Creating multiple files..."
for i in {1..10}; do
    echo "File $i" > "$TEST_MOUNT/file_$i.txt"
done
FILE_COUNT=$(ls "$TEST_MOUNT" | wc -l)
if [ "$FILE_COUNT" -ge 10 ]; then
    pass "Multiple files created ($FILE_COUNT files)"
else
    fail "Expected at least 10 files, found $FILE_COUNT"
fi

# Test 8: Unmount
echo "Test 8: Unmounting filesystem..."
if sudo umount "$TEST_MOUNT"; then
    sleep 1
    if ! mountpoint -q "$TEST_MOUNT" 2>/dev/null; then
        pass "Filesystem unmounted"
    else
        fail "Filesystem still mounted"
    fi
else
    fail "Failed to unmount filesystem"
fi

# Test 9: Check filesystem
echo "Test 9: Checking filesystem integrity..."
if sudo fsck.minifs "$TEST_IMG" --check-only &>/dev/null; then
    pass "Filesystem check passed"
else
    fail "Filesystem check failed"
fi

# Summary
echo ""
echo "=== Test Summary ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"

cleanup

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi

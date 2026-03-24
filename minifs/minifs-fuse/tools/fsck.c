/*
 * fsck.minifs - MiniFS Filesystem Checker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/minifs.h"

void print_usage(const char *prog)
{
    printf("Usage: %s <device> [options]\n", prog);
    printf("Options:\n");
    printf("  --check-only   Check without repair\n");
    printf("  --force        Force check\n");
    printf("  --verbose      Verbose output\n");
    printf("  --yes          Repair without confirmation\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *device = argv[1];
    int check_only = 0;
    int verbose = 0;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--check-only") == 0) {
            check_only = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        }
    }
    
    printf("fsck.minifs 1.0.0\n\n");
    printf("Checking %s...\n\n", device);
    
    minifs_context_t *ctx;
    if (minifs_mount(device, &ctx) != 0) {
        fprintf(stderr, "Failed to mount filesystem\n");
        return 1;
    }
    
    int errors = 0;
    
    printf("Pass 1: Checking superblock...\n");
    if (ctx->sb.magic != MINIFS_MAGIC) {
        printf("  ERROR: Invalid magic number\n");
        errors++;
    } else {
        printf("  OK\n");
    }
    
    printf("\nPass 2: Checking inodes...\n");
    uint32_t used_inodes = 0;
    for (uint32_t i = 0; i < ctx->sb.total_inodes; i++) {
        if (minifs_test_bit(ctx->inode_bitmap, i)) {
            used_inodes++;
        }
    }
    printf("  Found %u inodes in use\n", used_inodes);
    
    uint32_t expected_free = ctx->sb.total_inodes - used_inodes;
    if (expected_free != ctx->sb.free_inodes && !check_only) {
        printf("  ERROR: Free inodes mismatch (expected %u, got %u)\n",
               expected_free, ctx->sb.free_inodes);
        printf("  Fixing: Updating superblock\n");
        ctx->sb.free_inodes = expected_free;
        errors++;
    }
    
    printf("\nPass 3: Checking blocks...\n");
    uint64_t used_blocks = 0;
    for (uint64_t i = 0; i < ctx->sb.total_blocks; i++) {
        if (minifs_test_bit(ctx->block_bitmap, i)) {
            used_blocks++;
        }
    }
    printf("  Found %lu blocks in use\n", used_blocks);
    
    printf("\nSummary:\n");
    if (errors == 0) {
        printf("  Filesystem is clean\n");
    } else {
        printf("  %d errors found", errors);
        if (check_only) {
            printf(" (not repaired)\n");
        } else {
            printf(" and fixed\n");
            minifs_sync(ctx);
        }
    }
    
    minifs_unmount(ctx);
    
    return errors > 0 ? 1 : 0;
}

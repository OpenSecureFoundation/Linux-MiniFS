/*
 * debugfs.minifs - MiniFS Debug/Explorer Tool
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/minifs.h"

void print_usage(const char *prog)
{
    printf("Usage: %s <device> [options]\n", prog);
    printf("Options:\n");
    printf("  --stats          Show filesystem statistics\n");
    printf("  --superblock     Show superblock\n");
    printf("  --inode N        Show inode N\n");
    printf("  --fragmentation  Analyze fragmentation\n");
    printf("  --list-inodes    List all inodes\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *device = argv[1];
    
    minifs_context_t *ctx;
    if (minifs_mount(device, &ctx) != 0) {
        fprintf(stderr, "Failed to mount filesystem\n");
        return 1;
    }
    
    int action_taken = 0;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--stats") == 0) {
            printf("MiniFS Statistics\n");
            printf("=================\n");
            printf("Volume name:      %s\n", ctx->sb.volume_name);
            printf("Block size:       %u bytes\n", ctx->sb.block_size);
            printf("Total blocks:     %lu (%.1f MB)\n", 
                   ctx->sb.total_blocks,
                   (double)(ctx->sb.total_blocks * ctx->sb.block_size) / (1024 * 1024));
            printf("Free blocks:      %lu (%.1f%%)\n",
                   ctx->sb.free_blocks,
                   (double)ctx->sb.free_blocks / ctx->sb.total_blocks * 100);
            printf("Total inodes:     %u\n", ctx->sb.total_inodes);
            printf("Free inodes:      %u (%.1f%%)\n",
                   ctx->sb.free_inodes,
                   (double)ctx->sb.free_inodes / ctx->sb.total_inodes * 100);
            printf("Mount count:      %u\n", ctx->sb.mount_count);
            action_taken = 1;
        }
        else if (strcmp(argv[i], "--superblock") == 0) {
            minifs_dump_superblock(&ctx->sb);
            action_taken = 1;
        }
        else if (strcmp(argv[i], "--fragmentation") == 0) {
            minifs_check_fragmentation(ctx);
            action_taken = 1;
        }
        else if (strcmp(argv[i], "--inode") == 0 && i + 1 < argc) {
            uint32_t ino = atoi(argv[++i]);
            minifs_inode_t inode;
            if (minifs_read_inode(ctx, ino, &inode) == 0) {
                minifs_dump_inode(&inode, ino);
            } else {
                printf("Failed to read inode %u\n", ino);
            }
            action_taken = 1;
        }
        else if (strcmp(argv[i], "--list-inodes") == 0) {
            printf("=== Used Inodes ===\n");
            for (uint32_t i = 0; i < ctx->sb.total_inodes; i++) {
                if (minifs_test_bit(ctx->inode_bitmap, i)) {
                    printf("Inode %u: ", i);
                    minifs_inode_t inode;
                    if (minifs_read_inode(ctx, i, &inode) == 0) {
                        printf("size=%lu, links=%u\n", inode.size, inode.links_count);
                    } else {
                        printf("(read error)\n");
                    }
                }
            }
            action_taken = 1;
        }
    }
    
    if (!action_taken) {
        print_usage(argv[0]);
    }
    
    minifs_unmount(ctx);
    
    return 0;
}

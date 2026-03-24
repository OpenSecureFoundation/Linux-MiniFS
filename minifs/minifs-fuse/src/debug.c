/*
 * MiniFS - Debug Functions
 * Fonctions de débogage et analyse
 */

#include <stdio.h>
#include "../include/minifs.h"

void minifs_dump_superblock(const minifs_superblock_t *sb)
{
    if (!sb) return;
    
    printf("=== MiniFS Superblock ===\n");
    printf("Magic:           0x%08X\n", sb->magic);
    printf("Version:         %u\n", sb->version);
    printf("State:           %u (%s)\n", sb->state, 
           sb->state == MINIFS_VALID_FS ? "VALID" : "ERROR");
    printf("Block size:      %u bytes\n", sb->block_size);
    printf("Total blocks:    %lu\n", sb->total_blocks);
    printf("Total inodes:    %u\n", sb->total_inodes);
    printf("Free blocks:     %lu\n", sb->free_blocks);
    printf("Free inodes:     %u\n", sb->free_inodes);
    printf("Inode table:     block %lu\n", sb->inode_table_block);
    printf("Data start:      block %lu\n", sb->data_block_start);
    printf("Mount count:     %u\n", sb->mount_count);
    printf("Volume name:     %s\n", sb->volume_name);
    printf("=========================\n");
}

void minifs_dump_inode(const minifs_inode_t *inode, uint32_t ino)
{
    if (!inode) return;
    
    printf("=== Inode %u ===\n", ino);
    printf("Mode:            0%o\n", inode->mode);
    printf("Links:           %u\n", inode->links_count);
    printf("UID:             %u\n", inode->uid);
    printf("GID:             %u\n", inode->gid);
    printf("Size:            %lu bytes\n", inode->size);
    printf("Blocks:          %u\n", inode->blocks_count);
    printf("Direct blocks:   ");
    for (int i = 0; i < MINIFS_DIRECT_BLOCKS; i++) {
        if (inode->block[i] != 0) {
            printf("%lu ", inode->block[i]);
        }
    }
    printf("\n");
    if (inode->indirect_block != 0) {
        printf("Indirect block:  %lu\n", inode->indirect_block);
    }
    printf("=================\n");
}

int minifs_check_fragmentation(minifs_context_t *ctx)
{
    if (!ctx) return -1;
    
    printf("=== Fragmentation Analysis ===\n");
    
    /* Analyser la fragmentation des blocs libres */
    uint32_t free_sequences = 0;
    uint32_t in_free_seq = 0;
    
    for (uint64_t i = 0; i < ctx->sb.total_blocks; i++) {
        int is_free = !minifs_test_bit(ctx->block_bitmap, i);
        
        if (is_free && !in_free_seq) {
            free_sequences++;
            in_free_seq = 1;
        } else if (!is_free) {
            in_free_seq = 0;
        }
    }
    
    printf("Free sequences:  %u\n", free_sequences);
    printf("Free blocks:     %lu\n", ctx->sb.free_blocks);
    
    if (ctx->sb.free_blocks > 0) {
        float frag = ((float)free_sequences / ctx->sb.free_blocks) * 100;
        printf("Fragmentation:   %.1f%%\n", frag);
    }
    
    printf("==============================\n");
    
    return 0;
}

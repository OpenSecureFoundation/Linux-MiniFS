/*
 * MiniFS - Inode Management
 * Gestion des inodes (allocation, libération, lecture, écriture)
 */

#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include "../include/minifs.h"

/*
 * Allouer un nouvel inode
 */
uint32_t minifs_alloc_inode(minifs_context_t *ctx)
{
    if (!ctx) return 0;
    
    pthread_mutex_lock(&ctx->lock);
    
    if (ctx->sb.free_inodes == 0) {
        pthread_mutex_unlock(&ctx->lock);
        return 0;
    }
    
    uint32_t ino = minifs_find_first_zero_bit(ctx->inode_bitmap, ctx->sb.total_inodes);
    
    if (ino == 0 || ino >= ctx->sb.total_inodes) {
        pthread_mutex_unlock(&ctx->lock);
        return 0;
    }
    
    minifs_set_bit(ctx->inode_bitmap, ino);
    ctx->sb.free_inodes--;
    
    pthread_mutex_unlock(&ctx->lock);
    
    return ino;
}

/*
 * Libérer un inode
 */
void minifs_free_inode(minifs_context_t *ctx, uint32_t ino)
{
    if (!ctx || ino == 0 || ino >= ctx->sb.total_inodes || ino == MINIFS_ROOT_INODE) {
        return;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    minifs_clear_bit(ctx->inode_bitmap, ino);
    ctx->sb.free_inodes++;
    
    pthread_mutex_unlock(&ctx->lock);
}

/*
 * Lire un inode depuis le disque
 */
int minifs_read_inode(minifs_context_t *ctx, uint32_t ino, minifs_inode_t *inode)
{
    if (!ctx || !inode || ino >= ctx->sb.total_inodes) {
        return -EINVAL;
    }
    
    size_t inode_size = sizeof(minifs_inode_t);
    uint64_t inode_offset = ctx->sb.inode_table_block * MINIFS_BLOCK_SIZE + ino * inode_size;
    
    if (pread(ctx->fd, inode, inode_size, inode_offset) != (ssize_t)inode_size) {
        return -EIO;
    }
    
    return 0;
}

/*
 * Écrire un inode sur le disque
 */
int minifs_write_inode(minifs_context_t *ctx, uint32_t ino, const minifs_inode_t *inode)
{
    if (!ctx || !inode || ino >= ctx->sb.total_inodes) {
        return -EINVAL;
    }
    
    size_t inode_size = sizeof(minifs_inode_t);
    uint64_t inode_offset = ctx->sb.inode_table_block * MINIFS_BLOCK_SIZE + ino * inode_size;
    
    if (pwrite(ctx->fd, inode, inode_size, inode_offset) != (ssize_t)inode_size) {
        return -EIO;
    }
    
    return 0;
}

/*
 * Obtenir le numéro de bloc physique pour un index donné
 */
int minifs_get_block(minifs_context_t *ctx, minifs_inode_t *inode, uint32_t block_idx, uint64_t *block_num)
{
    if (!ctx || !inode || !block_num) {
        return -EINVAL;
    }
    
    if (block_idx < MINIFS_DIRECT_BLOCKS) {
        *block_num = inode->block[block_idx];
        return 0;
    }
    
    uint32_t indirect_idx = block_idx - MINIFS_DIRECT_BLOCKS;
    
    if (inode->indirect_block == 0) {
        *block_num = 0;
        return 0;
    }
    
    uint64_t *indirect_table = malloc(MINIFS_BLOCK_SIZE);
    if (!indirect_table) {
        return -ENOMEM;
    }
    
    if (minifs_read_block(ctx, inode->indirect_block, indirect_table) != 0) {
        free(indirect_table);
        return -EIO;
    }
    
    *block_num = indirect_table[indirect_idx];
    free(indirect_table);
    
    return 0;
}

/*
 * MiniFS - Block Allocation
 * Gestion de l'allocation et libération de blocs
 */

#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "../include/minifs.h"

/*
 * Allouer un nouveau bloc
 */
uint64_t minifs_alloc_block(minifs_context_t *ctx)
{
    if (!ctx) return 0;
    
    pthread_mutex_lock(&ctx->lock);
    
    if (ctx->sb.free_blocks == 0) {
        pthread_mutex_unlock(&ctx->lock);
        return 0;
    }
    
    uint64_t block = minifs_find_first_zero_bit(ctx->block_bitmap, ctx->sb.total_blocks);
    
    if (block == 0 || block >= ctx->sb.total_blocks) {
        pthread_mutex_unlock(&ctx->lock);
        return 0;
    }
    
    minifs_set_bit(ctx->block_bitmap, block);
    ctx->sb.free_blocks--;
    
    pthread_mutex_unlock(&ctx->lock);
    
    uint8_t *zero_block = calloc(1, MINIFS_BLOCK_SIZE);
    if (zero_block) {
        pwrite(ctx->fd, zero_block, MINIFS_BLOCK_SIZE, block * MINIFS_BLOCK_SIZE);
        free(zero_block);
    }
    
    return block;
}

/*
 * Libérer un bloc
 */
void minifs_free_block(minifs_context_t *ctx, uint64_t block)
{
    if (!ctx || block < ctx->sb.data_block_start || block >= ctx->sb.total_blocks) {
        return;
    }
    
    pthread_mutex_lock(&ctx->lock);
    
    minifs_clear_bit(ctx->block_bitmap, block);
    ctx->sb.free_blocks++;
    
    pthread_mutex_unlock(&ctx->lock);
}

/*
 * Lire un bloc depuis le disque
 */
int minifs_read_block(minifs_context_t *ctx, uint64_t block, void *buffer)
{
    if (!ctx || !buffer || block >= ctx->sb.total_blocks) {
        return -EINVAL;
    }
    
    off_t offset = block * MINIFS_BLOCK_SIZE;
    
    if (pread(ctx->fd, buffer, MINIFS_BLOCK_SIZE, offset) != MINIFS_BLOCK_SIZE) {
        return -EIO;
    }
    
    return 0;
}

/*
 * Écrire un bloc sur le disque
 */
int minifs_write_block(minifs_context_t *ctx, uint64_t block, const void *buffer)
{
    if (!ctx || !buffer || block >= ctx->sb.total_blocks) {
        return -EINVAL;
    }
    
    off_t offset = block * MINIFS_BLOCK_SIZE;
    
    if (pwrite(ctx->fd, buffer, MINIFS_BLOCK_SIZE, offset) != MINIFS_BLOCK_SIZE) {
        return -EIO;
    }
    
    return 0;
}

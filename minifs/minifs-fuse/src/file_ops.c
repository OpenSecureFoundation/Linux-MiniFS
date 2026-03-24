/*
 * MiniFS - File Operations
 * Opérations sur les fichiers (création, lecture, écriture, suppression)
 */

#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/minifs.h"

int minifs_create_file(minifs_context_t *ctx, const char *path, mode_t mode, uint32_t *ino_out)
{
    if (!ctx || !path || !ino_out) return -EINVAL;
    
    const char *filename = strrchr(path, '/');
    if (!filename) filename = path;
    else filename++;
    
    uint32_t parent_ino = MINIFS_ROOT_INODE;
    
    uint32_t existing_ino;
    if (minifs_lookup(ctx, parent_ino, filename, &existing_ino) == 0) {
        return -EEXIST;
    }
    
    uint32_t ino = minifs_alloc_inode(ctx);
    if (ino == 0) {
        return -ENOSPC;
    }
    
    minifs_inode_t inode;
    memset(&inode, 0, sizeof(inode));
    
    inode.mode = S_IFREG | (mode & 0777);
    inode.links_count = 1;
    inode.uid = getuid();
    inode.gid = getgid();
    inode.size = 0;
    inode.atime = minifs_current_time();
    inode.mtime = inode.atime;
    inode.ctime = inode.atime;
    inode.blocks_count = 0;
    inode.flags = 0;
    
    if (minifs_write_inode(ctx, ino, &inode) != 0) {
        minifs_free_inode(ctx, ino);
        return -EIO;
    }
    
    if (minifs_add_dir_entry(ctx, parent_ino, filename, ino, MINIFS_FT_REG_FILE) != 0) {
        minifs_free_inode(ctx, ino);
        return -EIO;
    }
    
    *ino_out = ino;
    return 0;
}

int minifs_read_file(minifs_context_t *ctx, uint32_t ino, char *buf, size_t size, off_t offset)
{
    if (!ctx || !buf) return -EINVAL;
    
    minifs_inode_t inode;
    if (minifs_read_inode(ctx, ino, &inode) != 0) {
        return -EIO;
    }
    
    if (offset >= (off_t)inode.size) {
        return 0;
    }
    
    if (offset + size > inode.size) {
        size = inode.size - offset;
    }
    
    uint32_t start_block = offset / MINIFS_BLOCK_SIZE;
    uint32_t end_block = (offset + size - 1) / MINIFS_BLOCK_SIZE;
    
    size_t bytes_read = 0;
    uint8_t *block_buffer = malloc(MINIFS_BLOCK_SIZE);
    if (!block_buffer) return -ENOMEM;
    
    for (uint32_t i = start_block; i <= end_block; i++) {
        uint64_t block_num;
        if (minifs_get_block(ctx, &inode, i, &block_num) != 0) {
            free(block_buffer);
            return -EIO;
        }
        
        if (block_num == 0) {
            memset(block_buffer, 0, MINIFS_BLOCK_SIZE);
        } else {
            if (minifs_read_block(ctx, block_num, block_buffer) != 0) {
                free(block_buffer);
                return -EIO;
            }
        }
        
        size_t block_start = (i == start_block) ? (offset % MINIFS_BLOCK_SIZE) : 0;
        size_t block_end = (i == end_block) ? 
            ((offset + size - 1) % MINIFS_BLOCK_SIZE) + 1 : MINIFS_BLOCK_SIZE;
        size_t block_bytes = block_end - block_start;
        
        memcpy(buf + bytes_read, block_buffer + block_start, block_bytes);
        bytes_read += block_bytes;
    }
    
    free(block_buffer);
    
    inode.atime = minifs_current_time();
    minifs_write_inode(ctx, ino, &inode);
    
    return bytes_read;
}

int minifs_write_file(minifs_context_t *ctx, uint32_t ino, const char *buf, size_t size, off_t offset)
{
    if (!ctx || !buf) return -EINVAL;
    
    minifs_inode_t inode;
    if (minifs_read_inode(ctx, ino, &inode) != 0) {
        return -EIO;
    }
    
    uint32_t start_block = offset / MINIFS_BLOCK_SIZE;
    uint32_t end_block = (offset + size - 1) / MINIFS_BLOCK_SIZE;
    
    size_t bytes_written = 0;
    uint8_t *block_buffer = malloc(MINIFS_BLOCK_SIZE);
    if (!block_buffer) return -ENOMEM;
    
    for (uint32_t i = start_block; i <= end_block; i++) {
        uint64_t block_num;
        if (minifs_get_block(ctx, &inode, i, &block_num) != 0) {
            free(block_buffer);
            return -EIO;
        }
        
        if (block_num == 0) {
            block_num = minifs_alloc_block(ctx);
            if (block_num == 0) {
                free(block_buffer);
                return -ENOSPC;
            }
            
            if (i < MINIFS_DIRECT_BLOCKS) {
                inode.block[i] = block_num;
                inode.blocks_count++;
            } else {
                free(block_buffer);
                return -ENOSPC;
            }
        }
        
        size_t block_start = (i == start_block) ? (offset % MINIFS_BLOCK_SIZE) : 0;
        size_t block_end = (i == end_block) ? 
            ((offset + size - 1) % MINIFS_BLOCK_SIZE) + 1 : MINIFS_BLOCK_SIZE;
        
        if (block_start > 0 || block_end < MINIFS_BLOCK_SIZE) {
            if (minifs_read_block(ctx, block_num, block_buffer) != 0) {
                memset(block_buffer, 0, MINIFS_BLOCK_SIZE);
            }
        }
        
        size_t block_bytes = block_end - block_start;
        memcpy(block_buffer + block_start, buf + bytes_written, block_bytes);
        
        if (minifs_write_block(ctx, block_num, block_buffer) != 0) {
            free(block_buffer);
            return -EIO;
        }
        
        bytes_written += block_bytes;
    }
    
    free(block_buffer);
    
    if (offset + bytes_written > inode.size) {
        inode.size = offset + bytes_written;
    }
    
    inode.mtime = minifs_current_time();
    inode.ctime = inode.mtime;
    
    minifs_write_inode(ctx, ino, &inode);
    
    return bytes_written;
}

int minifs_truncate_file(minifs_context_t *ctx, uint32_t ino, off_t size)
{
    if (!ctx) return -EINVAL;
    
    minifs_inode_t inode;
    if (minifs_read_inode(ctx, ino, &inode) != 0) {
        return -EIO;
    }
    
    if (size == (off_t)inode.size) {
        return 0;
    }
    
    if (size < (off_t)inode.size) {
        uint32_t last_block = (size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
        uint32_t total_blocks = (inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
        
        for (uint32_t i = last_block; i < total_blocks && i < MINIFS_DIRECT_BLOCKS; i++) {
            if (inode.block[i] != 0) {
                minifs_free_block(ctx, inode.block[i]);
                inode.block[i] = 0;
                inode.blocks_count--;
            }
        }
    }
    
    inode.size = size;
    inode.mtime = minifs_current_time();
    inode.ctime = inode.mtime;
    
    return minifs_write_inode(ctx, ino, &inode);
}

int minifs_unlink_file(minifs_context_t *ctx, const char *path)
{
    if (!ctx || !path) return -EINVAL;
    
    const char *filename = strrchr(path, '/');
    if (!filename) filename = path;
    else filename++;
    
    uint32_t parent_ino = MINIFS_ROOT_INODE;
    
    int ino = minifs_remove_dir_entry(ctx, parent_ino, filename);
    if (ino < 0) {
        return ino;
    }
    
    minifs_inode_t inode;
    if (minifs_read_inode(ctx, ino, &inode) != 0) {
        return -EIO;
    }
    
    inode.links_count--;
    
    if (inode.links_count == 0) {
        for (int i = 0; i < MINIFS_DIRECT_BLOCKS; i++) {
            if (inode.block[i] != 0) {
                minifs_free_block(ctx, inode.block[i]);
            }
        }
        
        minifs_free_inode(ctx, ino);
    } else {
        inode.ctime = minifs_current_time();
        minifs_write_inode(ctx, ino, &inode);
    }
    
    return 0;
}

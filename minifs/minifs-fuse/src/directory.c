/*
 * MiniFS - Directory Operations
 * Opérations sur les répertoires
 */

#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../include/minifs.h"

int minifs_readdir(minifs_context_t *ctx, uint32_t dir_ino, void *buf,
                   int (*filler)(void *, const char *, const struct stat *, off_t))
{
    if (!ctx || !filler) return -EINVAL;
    
    minifs_inode_t inode;
    if (minifs_read_inode(ctx, dir_ino, &inode) != 0) {
        return -EIO;
    }
    
    if (!S_ISDIR(inode.mode)) {
        return -ENOTDIR;
    }
    
    uint32_t num_blocks = (inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    
    for (uint32_t i = 0; i < num_blocks; i++) {
        uint64_t block_num;
        if (minifs_get_block(ctx, &inode, i, &block_num) != 0) {
            continue;
        }
        
        if (block_num == 0) continue;
        
        uint8_t *block_data = malloc(MINIFS_BLOCK_SIZE);
        if (!block_data) return -ENOMEM;
        
        if (minifs_read_block(ctx, block_num, block_data) != 0) {
            free(block_data);
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < MINIFS_BLOCK_SIZE) {
            minifs_dir_entry_t *entry = (minifs_dir_entry_t *)(block_data + offset);
            
            if (entry->inode == 0 || entry->rec_len == 0) {
                break;
            }
            
            struct stat st;
            memset(&st, 0, sizeof(st));
            
            minifs_inode_t entry_inode;
            if (minifs_read_inode(ctx, entry->inode, &entry_inode) == 0) {
                st.st_mode = entry_inode.mode;
                st.st_nlink = entry_inode.links_count;
                st.st_size = entry_inode.size;
            }
            
            filler(buf, entry->name, &st, 0);
            
            offset += entry->rec_len;
        }
        
        free(block_data);
    }
    
    return 0;
}

int minifs_lookup(minifs_context_t *ctx, uint32_t dir_ino, const char *name, uint32_t *ino)
{
    if (!ctx || !name || !ino) return -EINVAL;
    
    minifs_inode_t inode;
    if (minifs_read_inode(ctx, dir_ino, &inode) != 0) {
        return -EIO;
    }
    
    if (!S_ISDIR(inode.mode)) {
        return -ENOTDIR;
    }
    
    uint32_t num_blocks = (inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    
    for (uint32_t i = 0; i < num_blocks; i++) {
        uint64_t block_num;
        if (minifs_get_block(ctx, &inode, i, &block_num) != 0) {
            continue;
        }
        
        if (block_num == 0) continue;
        
        uint8_t *block_data = malloc(MINIFS_BLOCK_SIZE);
        if (!block_data) return -ENOMEM;
        
        if (minifs_read_block(ctx, block_num, block_data) != 0) {
            free(block_data);
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < MINIFS_BLOCK_SIZE) {
            minifs_dir_entry_t *entry = (minifs_dir_entry_t *)(block_data + offset);
            
            if (entry->inode == 0 || entry->rec_len == 0) {
                break;
            }
            
            if (strcmp(entry->name, name) == 0) {
                *ino = entry->inode;
                free(block_data);
                return 0;
            }
            
            offset += entry->rec_len;
        }
        
        free(block_data);
    }
    
    return -ENOENT;
}

int minifs_add_dir_entry(minifs_context_t *ctx, uint32_t dir_ino, const char *name,
                          uint32_t ino, uint8_t file_type)
{
    if (!ctx || !name || strlen(name) > MINIFS_MAX_FILENAME) {
        return -EINVAL;
    }
    
    uint32_t existing_ino;
    if (minifs_lookup(ctx, dir_ino, name, &existing_ino) == 0) {
        return -EEXIST;
    }
    
    minifs_inode_t dir_inode;
    if (minifs_read_inode(ctx, dir_ino, &dir_inode) != 0) {
        return -EIO;
    }
    
    uint16_t entry_size = sizeof(minifs_dir_entry_t);
    entry_size = (entry_size + 7) & ~7;
    
    uint32_t num_blocks = (dir_inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    
    for (uint32_t i = 0; i < num_blocks; i++) {
        uint64_t block_num;
        if (minifs_get_block(ctx, &dir_inode, i, &block_num) != 0) {
            continue;
        }
        
        if (block_num == 0) continue;
        
        uint8_t *block_data = malloc(MINIFS_BLOCK_SIZE);
        if (!block_data) return -ENOMEM;
        
        if (minifs_read_block(ctx, block_num, block_data) != 0) {
            free(block_data);
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < MINIFS_BLOCK_SIZE) {
            minifs_dir_entry_t *entry = (minifs_dir_entry_t *)(block_data + offset);
            
            if (entry->inode == 0 || entry->rec_len == 0) {
                break;
            }
            
            offset += entry->rec_len;
        }
        
        if (offset + entry_size <= MINIFS_BLOCK_SIZE) {
            minifs_dir_entry_t *new_entry = (minifs_dir_entry_t *)(block_data + offset);
            new_entry->inode = ino;
            new_entry->rec_len = MINIFS_BLOCK_SIZE - offset;
            new_entry->name_len = strlen(name);
            new_entry->file_type = file_type;
            strncpy(new_entry->name, name, MINIFS_MAX_FILENAME);
            
            if (minifs_write_block(ctx, block_num, block_data) != 0) {
                free(block_data);
                return -EIO;
            }
            
            free(block_data);
            
            dir_inode.mtime = minifs_current_time();
            minifs_write_inode(ctx, dir_ino, &dir_inode);
            
            return 0;
        }
        
        free(block_data);
    }
    
    uint64_t new_block = minifs_alloc_block(ctx);
    if (new_block == 0) {
        return -ENOSPC;
    }
    
    uint8_t *block_data = calloc(1, MINIFS_BLOCK_SIZE);
    if (!block_data) {
        minifs_free_block(ctx, new_block);
        return -ENOMEM;
    }
    
    minifs_dir_entry_t *new_entry = (minifs_dir_entry_t *)block_data;
    new_entry->inode = ino;
    new_entry->rec_len = MINIFS_BLOCK_SIZE;
    new_entry->name_len = strlen(name);
    new_entry->file_type = file_type;
    strncpy(new_entry->name, name, MINIFS_MAX_FILENAME);
    
    if (minifs_write_block(ctx, new_block, block_data) != 0) {
        free(block_data);
        minifs_free_block(ctx, new_block);
        return -EIO;
    }
    
    free(block_data);
    
    if (num_blocks < MINIFS_DIRECT_BLOCKS) {
        dir_inode.block[num_blocks] = new_block;
    } else {
        return -ENOSPC;
    }
    
    dir_inode.size += MINIFS_BLOCK_SIZE;
    dir_inode.blocks_count++;
    dir_inode.mtime = minifs_current_time();
    
    minifs_write_inode(ctx, dir_ino, &dir_inode);
    
    return 0;
}

int minifs_remove_dir_entry(minifs_context_t *ctx, uint32_t dir_ino, const char *name)
{
    if (!ctx || !name) return -EINVAL;
    
    minifs_inode_t inode;
    if (minifs_read_inode(ctx, dir_ino, &inode) != 0) {
        return -EIO;
    }
    
    uint32_t num_blocks = (inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    
    for (uint32_t i = 0; i < num_blocks; i++) {
        uint64_t block_num;
        if (minifs_get_block(ctx, &inode, i, &block_num) != 0) {
            continue;
        }
        
        if (block_num == 0) continue;
        
        uint8_t *block_data = malloc(MINIFS_BLOCK_SIZE);
        if (!block_data) return -ENOMEM;
        
        if (minifs_read_block(ctx, block_num, block_data) != 0) {
            free(block_data);
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < MINIFS_BLOCK_SIZE) {
            minifs_dir_entry_t *entry = (minifs_dir_entry_t *)(block_data + offset);
            
            if (entry->inode == 0 || entry->rec_len == 0) {
                break;
            }
            
            if (strcmp(entry->name, name) == 0) {
                uint32_t found_ino = entry->inode;
                
                entry->inode = 0;
                
                if (minifs_write_block(ctx, block_num, block_data) != 0) {
                    free(block_data);
                    return -EIO;
                }
                
                free(block_data);
                
                inode.mtime = minifs_current_time();
                minifs_write_inode(ctx, dir_ino, &inode);
                
                return found_ino;
            }
            
            offset += entry->rec_len;
        }
        
        free(block_data);
    }
    
    return -ENOENT;
}

/*
 * MiniFS - Minimal File System Implementation using FUSE
 */

#ifndef MINIFS_H
#define MINIFS_H

#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>

/* Magic number */
#define MINIFS_MAGIC 0x4D494E46

/* Version */
#define MINIFS_VERSION 1

/* Block size (4KB) */
#define MINIFS_BLOCK_SIZE 4096

/* Maximum filename length */
#define MINIFS_MAX_FILENAME 255

/* Number of direct block pointers in inode */
#define MINIFS_DIRECT_BLOCKS 12

/* Root inode number */
#define MINIFS_ROOT_INODE 1

/* Maximum file size */
#define MINIFS_MAX_FILE_SIZE ((MINIFS_DIRECT_BLOCKS + (MINIFS_BLOCK_SIZE / sizeof(uint64_t))) * MINIFS_BLOCK_SIZE)

/* File system states */
#define MINIFS_VALID_FS 0x0001
#define MINIFS_ERROR_FS 0x0002

/* Forward declaration */
struct stat;

/*
 * Superblock structure (1024 bytes)
 */
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t state;
    uint32_t block_size;
    uint64_t total_blocks;
    uint32_t total_inodes;
    uint64_t free_blocks;
    uint32_t free_inodes;
    uint64_t inode_table_block;
    uint64_t data_block_start;
    uint64_t mount_time;
    uint64_t write_time;
    uint16_t mount_count;
    uint16_t max_mount_count;
    char volume_name[16];
    uint8_t reserved[948];
} minifs_superblock_t;

/*
 * Inode structure (256 bytes)
 */
typedef struct {
    uint16_t mode;
    uint16_t links_count;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
    uint32_t blocks_count;
    uint32_t flags;
    uint64_t block[MINIFS_DIRECT_BLOCKS];
    uint64_t indirect_block;
    uint8_t reserved[80];
} minifs_inode_t;

/*
 * Directory entry structure
 */
typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char name[MINIFS_MAX_FILENAME + 1];
} minifs_dir_entry_t;

/* File types */
#define MINIFS_FT_UNKNOWN  0
#define MINIFS_FT_REG_FILE 1
#define MINIFS_FT_DIR      2
#define MINIFS_FT_CHRDEV   3
#define MINIFS_FT_BLKDEV   4
#define MINIFS_FT_FIFO     5
#define MINIFS_FT_SOCK     6
#define MINIFS_FT_SYMLINK  7

/*
 * In-memory context
 */
typedef struct {
    int fd;
    minifs_superblock_t sb;
    uint8_t *inode_bitmap;
    uint8_t *block_bitmap;
    size_t inode_bitmap_size;
    size_t block_bitmap_size;
    pthread_mutex_t lock;
} minifs_context_t;

/* Function prototypes */

/* filesystem.c */
int minifs_format(const char *device, uint64_t size, uint32_t num_inodes);
int minifs_mount(const char *device, minifs_context_t **ctx);
void minifs_unmount(minifs_context_t *ctx);
int minifs_sync(minifs_context_t *ctx);

/* inode.c */
uint32_t minifs_alloc_inode(minifs_context_t *ctx);
void minifs_free_inode(minifs_context_t *ctx, uint32_t ino);
int minifs_read_inode(minifs_context_t *ctx, uint32_t ino, minifs_inode_t *inode);
int minifs_write_inode(minifs_context_t *ctx, uint32_t ino, const minifs_inode_t *inode);
int minifs_get_block(minifs_context_t *ctx, minifs_inode_t *inode, uint32_t block_idx, uint64_t *block_num);

/* block.c */
uint64_t minifs_alloc_block(minifs_context_t *ctx);
void minifs_free_block(minifs_context_t *ctx, uint64_t block);
int minifs_read_block(minifs_context_t *ctx, uint64_t block, void *buffer);
int minifs_write_block(minifs_context_t *ctx, uint64_t block, const void *buffer);

/* directory.c */
int minifs_readdir(minifs_context_t *ctx, uint32_t dir_ino, void *buf,
                   int (*filler)(void *, const char *, const struct stat *, off_t));
int minifs_lookup(minifs_context_t *ctx, uint32_t dir_ino, const char *name, uint32_t *ino);
int minifs_add_dir_entry(minifs_context_t *ctx, uint32_t dir_ino, const char *name,
                          uint32_t ino, uint8_t file_type);
int minifs_remove_dir_entry(minifs_context_t *ctx, uint32_t dir_ino, const char *name);

/* file_ops.c */
int minifs_create_file(minifs_context_t *ctx, const char *path, mode_t mode, uint32_t *ino);
int minifs_read_file(minifs_context_t *ctx, uint32_t ino, char *buf, size_t size, off_t offset);
int minifs_write_file(minifs_context_t *ctx, uint32_t ino, const char *buf, size_t size, off_t offset);
int minifs_truncate_file(minifs_context_t *ctx, uint32_t ino, off_t size);
int minifs_unlink_file(minifs_context_t *ctx, const char *path);

/* utils.c */
void minifs_set_bit(uint8_t *bitmap, uint32_t bit);
void minifs_clear_bit(uint8_t *bitmap, uint32_t bit);
int minifs_test_bit(const uint8_t *bitmap, uint32_t bit);
uint32_t minifs_find_first_zero_bit(const uint8_t *bitmap, uint32_t size);
uint64_t minifs_current_time(void);
void minifs_inode_set_mode(minifs_inode_t *inode, mode_t mode);

/* debug.c */
void minifs_dump_superblock(const minifs_superblock_t *sb);
void minifs_dump_inode(const minifs_inode_t *inode, uint32_t ino);
int minifs_check_fragmentation(minifs_context_t *ctx);

#endif /* MINIFS_H */

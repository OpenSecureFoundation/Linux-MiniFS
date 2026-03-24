/*
 * MiniFS - Filesystem Management
 * Formatage, montage et synchronisation
 */

#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include "../include/minifs.h"

/*
 * Formater un périphérique/image avec MiniFS
 */
int minifs_format(const char *device, uint64_t size, uint32_t num_inodes)
{
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    /* Calculer la disposition */
    uint64_t total_blocks = size / MINIFS_BLOCK_SIZE;
    
    /* Superbloc : 1 bloc */
    uint64_t superblock_block = 0;
    
    /* Bitmap inodes : ceil(num_inodes / 8 / block_size) blocs */
    size_t inode_bitmap_size = (num_inodes + 7) / 8;
    uint64_t inode_bitmap_blocks = (inode_bitmap_size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    uint64_t inode_bitmap_block = 1;
    
    /* Table inodes : ceil(num_inodes * sizeof(inode) / block_size) blocs */
    size_t inode_size = sizeof(minifs_inode_t);
    uint64_t inode_table_blocks = (num_inodes * inode_size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    uint64_t inode_table_block = inode_bitmap_block + inode_bitmap_blocks;
    
    /* Bitmap blocs : ceil(total_blocks / 8 / block_size) blocs */
    size_t block_bitmap_size = (total_blocks + 7) / 8;
    uint64_t block_bitmap_blocks = (block_bitmap_size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    uint64_t block_bitmap_block = inode_table_block + inode_table_blocks;
    
    /* Blocs de données */
    uint64_t data_block_start = block_bitmap_block + block_bitmap_blocks;
    uint64_t data_blocks_count = total_blocks - data_block_start;
    
    printf("Formatting %s...\n", device);
    printf("Block size: %d bytes\n", MINIFS_BLOCK_SIZE);
    printf("Total blocks: %lu (%.1f MB)\n", total_blocks, (double)size / (1024 * 1024));
    printf("Inode count: %u\n", num_inodes);
    printf("\nLayout:\n");
    printf("  Superblock:      1 block   (%lu)\n", superblock_block);
    printf("  Inode bitmap:    %lu blocks (%lu-%lu)\n", 
           inode_bitmap_blocks, inode_bitmap_block, inode_bitmap_block + inode_bitmap_blocks - 1);
    printf("  Inode table:     %lu blocks (%lu-%lu)\n",
           inode_table_blocks, inode_table_block, inode_table_block + inode_table_blocks - 1);
    printf("  Block bitmap:    %lu blocks (%lu-%lu)\n",
           block_bitmap_blocks, block_bitmap_block, block_bitmap_block + block_bitmap_blocks - 1);
    printf("  Data blocks:     %lu blocks (%lu-%lu)\n",
           data_blocks_count, data_block_start, total_blocks - 1);
    
    /* Créer le superbloc */
    minifs_superblock_t sb;
    memset(&sb, 0, sizeof(sb));
    
    sb.magic = MINIFS_MAGIC;
    sb.version = MINIFS_VERSION;
    sb.state = MINIFS_VALID_FS;
    sb.block_size = MINIFS_BLOCK_SIZE;
    sb.total_blocks = total_blocks;
    sb.total_inodes = num_inodes;
    sb.free_blocks = data_blocks_count - 1;
    sb.free_inodes = num_inodes - 1;
    sb.inode_table_block = inode_table_block;
    sb.data_block_start = data_block_start;
    sb.mount_time = 0;
    sb.write_time = time(NULL);
    sb.mount_count = 0;
    sb.max_mount_count = 20;
    strncpy(sb.volume_name, "MiniFS", sizeof(sb.volume_name));
    
    /* Écrire le superbloc */
    if (pwrite(fd, &sb, sizeof(sb), 0) != sizeof(sb)) {
        perror("write superblock");
        close(fd);
        return -1;
    }
    
    /* Créer et initialiser le bitmap inodes */
    uint8_t *inode_bitmap = calloc(1, inode_bitmap_blocks * MINIFS_BLOCK_SIZE);
    inode_bitmap[0] = 0x03;
    
    if (pwrite(fd, inode_bitmap, inode_bitmap_blocks * MINIFS_BLOCK_SIZE,
               inode_bitmap_block * MINIFS_BLOCK_SIZE) < 0) {
        perror("write inode bitmap");
        free(inode_bitmap);
        close(fd);
        return -1;
    }
    free(inode_bitmap);
    
    /* Créer la table inodes */
    minifs_inode_t *inodes = calloc(num_inodes, sizeof(minifs_inode_t));
    
    /* Initialiser l'inode racine (inode 1) */
    inodes[1].mode = S_IFDIR | 0755;
    inodes[1].links_count = 2;
    inodes[1].uid = getuid();
    inodes[1].gid = getgid();
    inodes[1].size = MINIFS_BLOCK_SIZE;
    inodes[1].atime = time(NULL);
    inodes[1].mtime = time(NULL);
    inodes[1].ctime = time(NULL);
    inodes[1].blocks_count = 1;
    inodes[1].flags = 0;
    inodes[1].block[0] = data_block_start;
    
    if (pwrite(fd, inodes, inode_table_blocks * MINIFS_BLOCK_SIZE,
               inode_table_block * MINIFS_BLOCK_SIZE) < 0) {
        perror("write inode table");
        free(inodes);
        close(fd);
        return -1;
    }
    free(inodes);
    
    /* Créer et initialiser le bitmap blocs */
    uint8_t *block_bitmap = calloc(1, block_bitmap_blocks * MINIFS_BLOCK_SIZE);
    for (uint64_t i = 0; i <= data_block_start; i++) {
        minifs_set_bit(block_bitmap, i);
    }
    
    if (pwrite(fd, block_bitmap, block_bitmap_blocks * MINIFS_BLOCK_SIZE,
               block_bitmap_block * MINIFS_BLOCK_SIZE) < 0) {
        perror("write block bitmap");
        free(block_bitmap);
        close(fd);
        return -1;
    }
    free(block_bitmap);
    
    /* Créer le répertoire racine */
    uint8_t *root_dir = calloc(1, MINIFS_BLOCK_SIZE);
    minifs_dir_entry_t *entries = (minifs_dir_entry_t *)root_dir;
    
    entries[0].inode = MINIFS_ROOT_INODE;
    entries[0].rec_len = 12;
    entries[0].name_len = 1;
    entries[0].file_type = MINIFS_FT_DIR;
    strcpy(entries[0].name, ".");
    
    minifs_dir_entry_t *dot_dot = (minifs_dir_entry_t *)(root_dir + 12);
    dot_dot->inode = MINIFS_ROOT_INODE;
    dot_dot->rec_len = MINIFS_BLOCK_SIZE - 12;
    dot_dot->name_len = 2;
    dot_dot->file_type = MINIFS_FT_DIR;
    strcpy(dot_dot->name, "..");
    
    if (pwrite(fd, root_dir, MINIFS_BLOCK_SIZE,
               data_block_start * MINIFS_BLOCK_SIZE) < 0) {
        perror("write root directory");
        free(root_dir);
        close(fd);
        return -1;
    }
    free(root_dir);
    
    close(fd);
    
    printf("\nAvailable space: %.1f MB\n", (double)(data_blocks_count - 1) * MINIFS_BLOCK_SIZE / (1024 * 1024));
    printf("Root directory created.\n\n");
    printf("Filesystem created successfully.\n");
    
    return 0;
}

/*
 * Monter un système de fichiers MiniFS
 */
int minifs_mount(const char *device, minifs_context_t **ctx_out)
{
    minifs_context_t *ctx = malloc(sizeof(minifs_context_t));
    if (!ctx) {
        return -ENOMEM;
    }
    
    memset(ctx, 0, sizeof(minifs_context_t));
    
    ctx->fd = open(device, O_RDWR);
    if (ctx->fd < 0) {
        perror("open device");
        free(ctx);
        return -1;
    }
    
    if (pread(ctx->fd, &ctx->sb, sizeof(minifs_superblock_t), 0) != sizeof(minifs_superblock_t)) {
        perror("read superblock");
        close(ctx->fd);
        free(ctx);
        return -1;
    }
    
    if (ctx->sb.magic != MINIFS_MAGIC) {
        fprintf(stderr, "Invalid magic number: 0x%08X (expected 0x%08X)\n",
                ctx->sb.magic, MINIFS_MAGIC);
        close(ctx->fd);
        free(ctx);
        return -1;
    }
    
    if (ctx->sb.version != MINIFS_VERSION) {
        fprintf(stderr, "Unsupported version: %u\n", ctx->sb.version);
        close(ctx->fd);
        free(ctx);
        return -1;
    }
    
    ctx->inode_bitmap_size = (ctx->sb.total_inodes + 7) / 8;
    ctx->inode_bitmap = malloc(ctx->inode_bitmap_size);
    if (!ctx->inode_bitmap) {
        close(ctx->fd);
        free(ctx);
        return -ENOMEM;
    }
    
    if (pread(ctx->fd, ctx->inode_bitmap, ctx->inode_bitmap_size,
              MINIFS_BLOCK_SIZE) < 0) {
        perror("read inode bitmap");
        free(ctx->inode_bitmap);
        close(ctx->fd);
        free(ctx);
        return -1;
    }
    
    ctx->block_bitmap_size = (ctx->sb.total_blocks + 7) / 8;
    ctx->block_bitmap = malloc(ctx->block_bitmap_size);
    if (!ctx->block_bitmap) {
        free(ctx->inode_bitmap);
        close(ctx->fd);
        free(ctx);
        return -ENOMEM;
    }
    
    uint64_t block_bitmap_block = ctx->sb.inode_table_block +
        ((ctx->sb.total_inodes * sizeof(minifs_inode_t) + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE);
    
    if (pread(ctx->fd, ctx->block_bitmap, ctx->block_bitmap_size,
              block_bitmap_block * MINIFS_BLOCK_SIZE) < 0) {
        perror("read block bitmap");
        free(ctx->block_bitmap);
        free(ctx->inode_bitmap);
        close(ctx->fd);
        free(ctx);
        return -1;
    }
    
    pthread_mutex_init(&ctx->lock, NULL);
    
    ctx->sb.mount_count++;
    ctx->sb.mount_time = time(NULL);
    
    *ctx_out = ctx;
    
    return 0;
}

/*
 * Démonter le système de fichiers
 */
void minifs_unmount(minifs_context_t *ctx)
{
    if (!ctx) return;
    
    minifs_sync(ctx);
    
    if (ctx->inode_bitmap) free(ctx->inode_bitmap);
    if (ctx->block_bitmap) free(ctx->block_bitmap);
    
    pthread_mutex_destroy(&ctx->lock);
    
    if (ctx->fd >= 0) {
        close(ctx->fd);
    }
    
    free(ctx);
}

/*
 * Synchroniser le système de fichiers sur disque
 */
int minifs_sync(minifs_context_t *ctx)
{
    if (!ctx) return -EINVAL;
    
    pthread_mutex_lock(&ctx->lock);
    
    ctx->sb.write_time = time(NULL);
    if (pwrite(ctx->fd, &ctx->sb, sizeof(minifs_superblock_t), 0) != sizeof(minifs_superblock_t)) {
        pthread_mutex_unlock(&ctx->lock);
        return -EIO;
    }
    
    if (pwrite(ctx->fd, ctx->inode_bitmap, ctx->inode_bitmap_size,
               MINIFS_BLOCK_SIZE) < 0) {
        pthread_mutex_unlock(&ctx->lock);
        return -EIO;
    }
    
    uint64_t block_bitmap_block = ctx->sb.inode_table_block +
        ((ctx->sb.total_inodes * sizeof(minifs_inode_t) + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE);
    
    if (pwrite(ctx->fd, ctx->block_bitmap, ctx->block_bitmap_size,
               block_bitmap_block * MINIFS_BLOCK_SIZE) < 0) {
        pthread_mutex_unlock(&ctx->lock);
        return -EIO;
    }
    
    fsync(ctx->fd);
    
    pthread_mutex_unlock(&ctx->lock);
    
    return 0;
}

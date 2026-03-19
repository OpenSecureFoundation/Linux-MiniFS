/**
 * minifs_core.c — Implémentation de la couche bas-niveau MiniFS
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "minifs.h"
#include "minifs_core.h"

/* ============================================================
 * Contexte global
 * ============================================================ */
minifs_ctx_t g_ctx = { .fd = -1 };

/* ============================================================
 * Initialisation
 * ============================================================ */

int minifs_open(const char *path)
{
    g_ctx.fd = open(path, O_RDWR);
    if (g_ctx.fd < 0) {
        perror("minifs_open: open");
        return -errno;
    }

    /* Charger le superbloc */
    if (minifs_read_block(MINIFS_SUPERBLOCK_BLOCK, &g_ctx.sb) < 0) {
        close(g_ctx.fd);
        g_ctx.fd = -1;
        return -EIO;
    }

    /* Vérifier la signature */
    if (g_ctx.sb.magic != MINIFS_MAGIC) {
        fprintf(stderr, "minifs_open: magic invalide (0x%08X)\n", g_ctx.sb.magic);
        close(g_ctx.fd);
        g_ctx.fd = -1;
        return -EINVAL;
    }

    return 0;
}

void minifs_close(void)
{
    if (g_ctx.fd >= 0) {
        minifs_sync_superblock();
        close(g_ctx.fd);
        g_ctx.fd = -1;
    }
}

/* ============================================================
 * Blocs bas-niveau
 * ============================================================ */

int minifs_read_block(uint32_t block_no, void *buf)
{
    off_t offset = (off_t)block_no * MINIFS_BLOCK_SIZE;
    if (lseek(g_ctx.fd, offset, SEEK_SET) < 0)
        return -errno;
    ssize_t n = read(g_ctx.fd, buf, MINIFS_BLOCK_SIZE);
    if (n != MINIFS_BLOCK_SIZE)
        return -EIO;
    return 0;
}

int minifs_write_block(uint32_t block_no, const void *buf)
{
    off_t offset = (off_t)block_no * MINIFS_BLOCK_SIZE;
    if (lseek(g_ctx.fd, offset, SEEK_SET) < 0)
        return -errno;
    ssize_t n = write(g_ctx.fd, buf, MINIFS_BLOCK_SIZE);
    if (n != MINIFS_BLOCK_SIZE)
        return -EIO;
    return 0;
}

/* ============================================================
 * Bitmap des inodes
 * ============================================================ */

int minifs_inode_is_used(uint32_t ino)
{
    if (ino >= MINIFS_MAX_INODES) return 0;
    uint8_t bitmap[MINIFS_BLOCK_SIZE];
    if (minifs_read_block(MINIFS_INODE_BITMAP_BLOCK, bitmap) < 0) return 0;
    return (bitmap[ino / 8] >> (ino % 8)) & 0x01;
}

int minifs_inode_alloc(uint32_t ino)
{
    if (ino >= MINIFS_MAX_INODES) return -EINVAL;
    uint8_t bitmap[MINIFS_BLOCK_SIZE];
    if (minifs_read_block(MINIFS_INODE_BITMAP_BLOCK, bitmap) < 0) return -EIO;
    bitmap[ino / 8] |= (1 << (ino % 8));
    if (minifs_write_block(MINIFS_INODE_BITMAP_BLOCK, bitmap) < 0) return -EIO;
    if (g_ctx.sb.free_inodes > 0) g_ctx.sb.free_inodes--;
    return 0;
}

int minifs_inode_free(uint32_t ino)
{
    if (ino >= MINIFS_MAX_INODES) return -EINVAL;
    uint8_t bitmap[MINIFS_BLOCK_SIZE];
    if (minifs_read_block(MINIFS_INODE_BITMAP_BLOCK, bitmap) < 0) return -EIO;
    bitmap[ino / 8] &= ~(1 << (ino % 8));
    if (minifs_write_block(MINIFS_INODE_BITMAP_BLOCK, bitmap) < 0) return -EIO;
    g_ctx.sb.free_inodes++;
    return 0;
}

int minifs_inode_find_free(void)
{
    uint8_t bitmap[MINIFS_BLOCK_SIZE];
    if (minifs_read_block(MINIFS_INODE_BITMAP_BLOCK, bitmap) < 0) return -1;
    for (uint32_t i = 0; i < MINIFS_MAX_INODES; i++) {
        if (!((bitmap[i / 8] >> (i % 8)) & 0x01))
            return (int)i;
    }
    return -1; /* Plus d'inode libre */
}

/* ============================================================
 * Bitmap des blocs de données
 * ============================================================ */

int minifs_block_alloc(void)
{
    uint8_t bitmap[MINIFS_BLOCK_SIZE];
    if (minifs_read_block(MINIFS_BLOCK_BITMAP_BLOCK, bitmap) < 0) return -1;

    uint32_t data_count = g_ctx.sb.total_blocks - g_ctx.sb.data_start;
    for (uint32_t i = 0; i < data_count; i++) {
        if (!((bitmap[i / 8] >> (i % 8)) & 0x01)) {
            /* Marquer le bloc comme utilisé */
            bitmap[i / 8] |= (1 << (i % 8));
            if (minifs_write_block(MINIFS_BLOCK_BITMAP_BLOCK, bitmap) < 0) return -1;
            if (g_ctx.sb.free_blocks > 0) g_ctx.sb.free_blocks--;
            /* Zéroïser le bloc alloué */
            uint8_t zero[MINIFS_BLOCK_SIZE];
            memset(zero, 0, sizeof(zero));
            uint32_t abs_block = g_ctx.sb.data_start + i;
            minifs_write_block(abs_block, zero);
            return (int)abs_block;
        }
    }
    return -1; /* Disque plein */
}

int minifs_block_free(uint32_t block_no)
{
    if (block_no < g_ctx.sb.data_start) return -EINVAL;
    uint32_t rel = block_no - g_ctx.sb.data_start;
    uint32_t data_count = g_ctx.sb.total_blocks - g_ctx.sb.data_start;
    if (rel >= data_count) return -EINVAL;

    uint8_t bitmap[MINIFS_BLOCK_SIZE];
    if (minifs_read_block(MINIFS_BLOCK_BITMAP_BLOCK, bitmap) < 0) return -EIO;
    bitmap[rel / 8] &= ~(1 << (rel % 8));
    if (minifs_write_block(MINIFS_BLOCK_BITMAP_BLOCK, bitmap) < 0) return -EIO;
    g_ctx.sb.free_blocks++;
    return 0;
}

/* ============================================================
 * Inodes
 * ============================================================ */

int minifs_read_inode(uint32_t ino, minifs_inode_t *inode)
{
    if (ino >= MINIFS_MAX_INODES) return -EINVAL;

    /* Calculer le bloc et l'offset dans la table */
    uint32_t byte_offset = ino * MINIFS_INODE_SIZE;
    uint32_t block_no    = MINIFS_INODE_TABLE_START + (byte_offset / MINIFS_BLOCK_SIZE);
    uint32_t offset_in   = byte_offset % MINIFS_BLOCK_SIZE;

    uint8_t buf[MINIFS_BLOCK_SIZE];
    if (minifs_read_block(block_no, buf) < 0) return -EIO;
    memcpy(inode, buf + offset_in, MINIFS_INODE_SIZE);
    return 0;
}

int minifs_write_inode(uint32_t ino, const minifs_inode_t *inode)
{
    if (ino >= MINIFS_MAX_INODES) return -EINVAL;

    uint32_t byte_offset = ino * MINIFS_INODE_SIZE;
    uint32_t block_no    = MINIFS_INODE_TABLE_START + (byte_offset / MINIFS_BLOCK_SIZE);
    uint32_t offset_in   = byte_offset % MINIFS_BLOCK_SIZE;

    uint8_t buf[MINIFS_BLOCK_SIZE];
    if (minifs_read_block(block_no, buf) < 0) return -EIO;
    memcpy(buf + offset_in, inode, MINIFS_INODE_SIZE);
    if (minifs_write_block(block_no, buf) < 0) return -EIO;
    return 0;
}

/* ============================================================
 * Répertoires
 * ============================================================ */

int minifs_dir_lookup(uint32_t dir_ino, const char *name)
{
    minifs_inode_t inode;
    if (minifs_read_inode(dir_ino, &inode) < 0)       return -EIO;
    if (inode.type != MINIFS_ITYPE_DIR)                return -ENOTDIR;

    uint8_t buf[MINIFS_BLOCK_SIZE];
    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++) {
        if (inode.direct[b] == 0) continue;
        if (minifs_read_block(inode.direct[b], buf) < 0) return -EIO;

        minifs_dirent_t *entries = (minifs_dirent_t *)buf;
        for (uint32_t i = 0; i < MINIFS_DIRENTS_PER_BLOCK; i++) {
            /* name[0] == '\0' indique une entree libre (evite le conflit
               entre inode 0 = racine et inode 0 = libre) */
            if (entries[i].name[0] == '\0') continue;
            if (strncmp(entries[i].name, name, MINIFS_NAME_MAX) == 0)
                return (int)entries[i].inode;
        }
    }
    return -ENOENT;
}

int minifs_path_to_ino(const char *path)
{
    if (strcmp(path, "/") == 0)
        return MINIFS_ROOT_INODE;

    /* Copier le chemin pour le tokeniser */
    char tmp[256];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    uint32_t cur_ino = MINIFS_ROOT_INODE;
    char *token = strtok(tmp, "/");
    while (token != NULL) {
        int ino = minifs_dir_lookup(cur_ino, token);
        if (ino < 0) return -ENOENT;
        cur_ino = (uint32_t)ino;
        token = strtok(NULL, "/");
    }
    return (int)cur_ino;
}

int minifs_dir_add_entry(uint32_t dir_ino, uint32_t ino, const char *name)
{
    minifs_inode_t dir_inode;
    if (minifs_read_inode(dir_ino, &dir_inode) < 0) return -EIO;
    if (dir_inode.type != MINIFS_ITYPE_DIR)         return -ENOTDIR;

    uint8_t buf[MINIFS_BLOCK_SIZE];

    /* Chercher un emplacement libre dans les blocs existants */
    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++) {
        if (dir_inode.direct[b] == 0) continue;
        if (minifs_read_block(dir_inode.direct[b], buf) < 0) return -EIO;

        minifs_dirent_t *entries = (minifs_dirent_t *)buf;
        for (uint32_t i = 0; i < MINIFS_DIRENTS_PER_BLOCK; i++) {
            if (entries[i].name[0] == '\0') { /* emplacement libre */
                entries[i].inode = ino;
                strncpy(entries[i].name, name, MINIFS_NAME_MAX - 1);
                entries[i].name[MINIFS_NAME_MAX - 1] = '\0';
                return minifs_write_block(dir_inode.direct[b], buf);
            }
        }
    }

    /* Aucun emplacement libre : allouer un nouveau bloc */
    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++) {
        if (dir_inode.direct[b] != 0) continue;

        int new_block = minifs_block_alloc();
        if (new_block < 0) return -ENOSPC;

        /* Le bloc est déjà zéroïsé par minifs_block_alloc() */
        if (minifs_read_block((uint32_t)new_block, buf) < 0) return -EIO;

        minifs_dirent_t *entries = (minifs_dirent_t *)buf;
        entries[0].inode = ino;
        strncpy(entries[0].name, name, MINIFS_NAME_MAX - 1);
        entries[0].name[MINIFS_NAME_MAX - 1] = '\0';

        if (minifs_write_block((uint32_t)new_block, buf) < 0) return -EIO;

        dir_inode.direct[b]  = (uint32_t)new_block;
        dir_inode.blocks_used++;
        dir_inode.size       += MINIFS_BLOCK_SIZE;
        dir_inode.mtime      = (uint32_t)time(NULL);
        return minifs_write_inode(dir_ino, &dir_inode);
    }

    return -ENOSPC; /* Répertoire trop grand */
}

int minifs_dir_remove_entry(uint32_t dir_ino, const char *name)
{
    minifs_inode_t dir_inode;
    if (minifs_read_inode(dir_ino, &dir_inode) < 0) return -EIO;

    uint8_t buf[MINIFS_BLOCK_SIZE];
    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++) {
        if (dir_inode.direct[b] == 0) continue;
        if (minifs_read_block(dir_inode.direct[b], buf) < 0) return -EIO;

        minifs_dirent_t *entries = (minifs_dirent_t *)buf;
        for (uint32_t i = 0; i < MINIFS_DIRENTS_PER_BLOCK; i++) {
            if (entries[i].name[0] == '\0') continue; /* entree libre, skip */
            if (strncmp(entries[i].name, name, MINIFS_NAME_MAX) == 0) {
                memset(&entries[i], 0, sizeof(minifs_dirent_t));
                return minifs_write_block(dir_inode.direct[b], buf);
            }
        }
    }
    return -ENOENT;
}

/* ============================================================
 * Persistance du superbloc
 * ============================================================ */

int minifs_sync_superblock(void)
{
    return minifs_write_block(MINIFS_SUPERBLOCK_BLOCK, &g_ctx.sb);
}

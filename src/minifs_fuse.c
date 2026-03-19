/**
 * minifs_fuse.c — Driver FUSE3 pour MiniFS
 *
 * Compilation (OpenSUSE) :
 *   sudo zypper install fuse3-devel
 *   make
 *
 * Montage :
 *   mkdir -p /tmp/minifs_mnt
 *   ./bin/minifs_fuse minifs.img /tmp/minifs_mnt
 *
 * Démontage :
 *   fusermount3 -u /tmp/minifs_mnt
 */

#define FUSE_USE_VERSION 31
#define _POSIX_C_SOURCE  200809L

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "minifs.h"
#include "minifs_core.h"

/* ============================================================
 * Helpers internes
 * ============================================================ */

/** Extraire le nom du dernier composant d'un chemin. */
static const char *basename_of(const char *path)
{
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}

/** Extraire le repertoire parent d'un chemin dans buf. */
static void parent_dir(const char *path, char *buf, size_t n)
{
    strncpy(buf, path, n - 1);
    buf[n - 1] = '\0';
    char *p = strrchr(buf, '/');
    if (p) {
        if (p == buf) *(p + 1) = '\0';
        else          *p = '\0';
    }
}

/* ============================================================
 * Callbacks FUSE
 * ============================================================ */

static int mfs_getattr(const char *path, struct stat *st,
                       struct fuse_file_info *fi)
{
    (void)fi;
    memset(st, 0, sizeof(*st));

    int ino = minifs_path_to_ino(path);
    if (ino < 0) return -ENOENT;

    minifs_inode_t inode;
    if (minifs_read_inode((uint32_t)ino, &inode) < 0) return -EIO;

    if (inode.type == MINIFS_ITYPE_DIR) {
        st->st_mode  = S_IFDIR | inode.permissions;
        st->st_nlink = 2;
    } else if (inode.type == MINIFS_ITYPE_FILE) {
        st->st_mode  = S_IFREG | inode.permissions;
        st->st_nlink = 1;
    } else {
        return -ENOENT;
    }

    st->st_ino    = (ino_t)ino;
    st->st_size   = inode.size;
    st->st_blocks = inode.blocks_used * (MINIFS_BLOCK_SIZE / 512);
    st->st_atime  = inode.atime;
    st->st_mtime  = inode.mtime;
    st->st_ctime  = inode.ctime;
    st->st_uid    = getuid();
    st->st_gid    = getgid();
    return 0;
}

static int mfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags)
{
    (void)offset; (void)fi; (void)flags;

    int dir_ino = minifs_path_to_ino(path);
    if (dir_ino < 0) return -ENOENT;

    minifs_inode_t dir_inode;
    if (minifs_read_inode((uint32_t)dir_ino, &dir_inode) < 0) return -EIO;
    if (dir_inode.type != MINIFS_ITYPE_DIR) return -ENOTDIR;

    filler(buf, ".",  NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    uint8_t block_buf[MINIFS_BLOCK_SIZE];
    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++) {
        if (dir_inode.direct[b] == 0) continue;
        if (minifs_read_block(dir_inode.direct[b], block_buf) < 0) return -EIO;

        minifs_dirent_t *entries = (minifs_dirent_t *)block_buf;
        for (uint32_t i = 0; i < MINIFS_DIRENTS_PER_BLOCK; i++) {
            if (entries[i].inode == 0) continue;
            if (strcmp(entries[i].name, ".")  == 0) continue;
            if (strcmp(entries[i].name, "..") == 0) continue;
            filler(buf, entries[i].name, NULL, 0, 0);
        }
    }
    return 0;
}

static int mfs_open(const char *path, struct fuse_file_info *fi)
{
    int ino = minifs_path_to_ino(path);
    if (ino < 0) return -ENOENT;

    minifs_inode_t inode;
    if (minifs_read_inode((uint32_t)ino, &inode) < 0) return -EIO;
    if (inode.type != MINIFS_ITYPE_FILE) return -EISDIR;

    fi->fh = (uint64_t)ino;
    inode.atime = (uint32_t)time(NULL);
    minifs_write_inode((uint32_t)ino, &inode);
    return 0;
}

static int mfs_read(const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi)
{
    (void)path;
    uint32_t ino = (uint32_t)fi->fh;

    minifs_inode_t inode;
    if (minifs_read_inode(ino, &inode) < 0) return -EIO;
    if (inode.type != MINIFS_ITYPE_FILE)    return -EISDIR;

    if ((uint32_t)offset >= inode.size) return 0;
    if (offset + (off_t)size > (off_t)inode.size)
        size = (size_t)(inode.size - (uint32_t)offset);

    size_t bytes_read = 0;
    uint8_t block_buf[MINIFS_BLOCK_SIZE];

    while (bytes_read < size) {
        uint32_t file_offset  = (uint32_t)offset + (uint32_t)bytes_read;
        uint32_t block_index  = file_offset / MINIFS_BLOCK_SIZE;
        uint32_t block_offset = file_offset % MINIFS_BLOCK_SIZE;

        if (block_index >= MINIFS_DIRECT_BLOCKS) break;

        uint32_t block_no = inode.direct[block_index];
        if (block_no == 0) break;

        if (minifs_read_block(block_no, block_buf) < 0) return -EIO;

        size_t can_read = MINIFS_BLOCK_SIZE - block_offset;
        size_t to_read  = size - bytes_read;
        if (to_read > can_read) to_read = can_read;

        memcpy(buf + bytes_read, block_buf + block_offset, to_read);
        bytes_read += to_read;
    }

    inode.atime = (uint32_t)time(NULL);
    minifs_write_inode(ino, &inode);
    return (int)bytes_read;
}

static int mfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    (void)path;
    uint32_t ino = (uint32_t)fi->fh;

    minifs_inode_t inode;
    if (minifs_read_inode(ino, &inode) < 0) return -EIO;
    if (inode.type != MINIFS_ITYPE_FILE)    return -EISDIR;

    size_t bytes_written = 0;
    uint8_t block_buf[MINIFS_BLOCK_SIZE];

    while (bytes_written < size) {
        uint32_t file_offset  = (uint32_t)offset + (uint32_t)bytes_written;
        uint32_t block_index  = file_offset / MINIFS_BLOCK_SIZE;
        uint32_t block_offset = file_offset % MINIFS_BLOCK_SIZE;

        if (block_index >= MINIFS_DIRECT_BLOCKS) break;

        if (inode.direct[block_index] == 0) {
            int new_block = minifs_block_alloc();
            if (new_block < 0)
                return bytes_written > 0 ? (int)bytes_written : -ENOSPC;
            inode.direct[block_index] = (uint32_t)new_block;
            inode.blocks_used++;
        }

        uint32_t block_no = inode.direct[block_index];
        if (minifs_read_block(block_no, block_buf) < 0) return -EIO;

        size_t can_write = MINIFS_BLOCK_SIZE - block_offset;
        size_t to_write  = size - bytes_written;
        if (to_write > can_write) to_write = can_write;

        memcpy(block_buf + block_offset, buf + bytes_written, to_write);
        if (minifs_write_block(block_no, block_buf) < 0) return -EIO;

        bytes_written += to_write;
    }

    uint32_t new_end = (uint32_t)offset + (uint32_t)bytes_written;
    if (new_end > inode.size) inode.size = new_end;
    inode.mtime = (uint32_t)time(NULL);
    if (minifs_write_inode(ino, &inode) < 0) return -EIO;
    minifs_sync_superblock();
    return (int)bytes_written;
}

static int mfs_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi)
{
    if (minifs_path_to_ino(path) >= 0) return -EEXIST;

    int new_ino = minifs_inode_find_free();
    if (new_ino < 0) return -ENOSPC;

    minifs_inode_t inode;
    memset(&inode, 0, sizeof(inode));
    inode.type        = MINIFS_ITYPE_FILE;
    inode.permissions = (uint16_t)(mode & 0xFFFF);
    inode.ctime       = (uint32_t)time(NULL);
    inode.mtime       = inode.ctime;
    inode.atime       = inode.ctime;

    if (minifs_write_inode((uint32_t)new_ino, &inode) < 0) return -EIO;
    if (minifs_inode_alloc((uint32_t)new_ino) < 0)         return -EIO;

    char parent[256];
    parent_dir(path, parent, sizeof(parent));
    int par_ino = minifs_path_to_ino(parent);
    if (par_ino < 0) return -ENOENT;

    int ret = minifs_dir_add_entry((uint32_t)par_ino, (uint32_t)new_ino,
                                   basename_of(path));
    if (ret < 0) return ret;

    minifs_sync_superblock();
    fi->fh = (uint64_t)new_ino;
    return 0;
}

static int mfs_mkdir(const char *path, mode_t mode)
{
    if (minifs_path_to_ino(path) >= 0) return -EEXIST;

    int new_ino = minifs_inode_find_free();
    if (new_ino < 0) return -ENOSPC;

    int new_block = minifs_block_alloc();
    if (new_block < 0) return -ENOSPC;

    minifs_inode_t inode;
    memset(&inode, 0, sizeof(inode));
    inode.type        = MINIFS_ITYPE_DIR;
    inode.permissions = (uint16_t)(mode & 0xFFFF);
    inode.size        = MINIFS_BLOCK_SIZE;
    inode.blocks_used = 1;
    inode.ctime       = (uint32_t)time(NULL);
    inode.mtime       = inode.ctime;
    inode.atime       = inode.ctime;
    inode.direct[0]   = (uint32_t)new_block;

    if (minifs_write_inode((uint32_t)new_ino, &inode) < 0) return -EIO;
    if (minifs_inode_alloc((uint32_t)new_ino) < 0)         return -EIO;

    /* Ecrire les entrees "." et ".." */
    uint8_t buf[MINIFS_BLOCK_SIZE];
    memset(buf, 0, sizeof(buf));
    minifs_dirent_t *entries = (minifs_dirent_t *)buf;
    entries[0].inode = (uint32_t)new_ino;
    strncpy(entries[0].name, ".", MINIFS_NAME_MAX - 1);

    char parent[256];
    parent_dir(path, parent, sizeof(parent));
    int par_ino = minifs_path_to_ino(parent);
    if (par_ino < 0) return -ENOENT;

    entries[1].inode = (uint32_t)par_ino;
    strncpy(entries[1].name, "..", MINIFS_NAME_MAX - 1);
    if (minifs_write_block((uint32_t)new_block, buf) < 0) return -EIO;

    int ret = minifs_dir_add_entry((uint32_t)par_ino, (uint32_t)new_ino,
                                   basename_of(path));
    if (ret < 0) return ret;

    minifs_sync_superblock();
    return 0;
}

static int mfs_unlink(const char *path)
{
    int ino = minifs_path_to_ino(path);
    if (ino < 0) return -ENOENT;

    minifs_inode_t inode;
    if (minifs_read_inode((uint32_t)ino, &inode) < 0) return -EIO;
    if (inode.type != MINIFS_ITYPE_FILE) return -EISDIR;

    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++)
        if (inode.direct[b] != 0)
            minifs_block_free(inode.direct[b]);

    memset(&inode, 0, sizeof(inode));
    minifs_write_inode((uint32_t)ino, &inode);
    minifs_inode_free((uint32_t)ino);

    char parent[256];
    parent_dir(path, parent, sizeof(parent));
    int par_ino = minifs_path_to_ino(parent);
    if (par_ino >= 0)
        minifs_dir_remove_entry((uint32_t)par_ino, basename_of(path));

    minifs_sync_superblock();
    return 0;
}

static int mfs_rmdir(const char *path)
{
    int ino = minifs_path_to_ino(path);
    if (ino < 0) return -ENOENT;

    minifs_inode_t inode;
    if (minifs_read_inode((uint32_t)ino, &inode) < 0) return -EIO;
    if (inode.type != MINIFS_ITYPE_DIR) return -ENOTDIR;

    uint8_t buf[MINIFS_BLOCK_SIZE];
    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++) {
        if (inode.direct[b] == 0) continue;
        if (minifs_read_block(inode.direct[b], buf) < 0) return -EIO;
        minifs_dirent_t *entries = (minifs_dirent_t *)buf;
        for (uint32_t i = 0; i < MINIFS_DIRENTS_PER_BLOCK; i++) {
            if (entries[i].inode == 0) continue;
            if (strcmp(entries[i].name, ".")  == 0) continue;
            if (strcmp(entries[i].name, "..") == 0) continue;
            return -ENOTEMPTY;
        }
    }

    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++)
        if (inode.direct[b] != 0)
            minifs_block_free(inode.direct[b]);

    memset(&inode, 0, sizeof(inode));
    minifs_write_inode((uint32_t)ino, &inode);
    minifs_inode_free((uint32_t)ino);

    char parent[256];
    parent_dir(path, parent, sizeof(parent));
    int par_ino = minifs_path_to_ino(parent);
    if (par_ino >= 0)
        minifs_dir_remove_entry((uint32_t)par_ino, basename_of(path));

    minifs_sync_superblock();
    return 0;
}

static int mfs_truncate(const char *path, off_t size,
                        struct fuse_file_info *fi)
{
    (void)fi;
    int ino = minifs_path_to_ino(path);
    if (ino < 0) return -ENOENT;

    minifs_inode_t inode;
    if (minifs_read_inode((uint32_t)ino, &inode) < 0) return -EIO;
    if (inode.type != MINIFS_ITYPE_FILE) return -EISDIR;

    for (uint32_t b = 0; b < MINIFS_DIRECT_BLOCKS; b++) {
        if (inode.direct[b] != 0 && (off_t)(b * MINIFS_BLOCK_SIZE) >= size) {
            minifs_block_free(inode.direct[b]);
            inode.direct[b] = 0;
            if (inode.blocks_used > 0) inode.blocks_used--;
        }
    }

    inode.size  = (uint32_t)size;
    inode.mtime = (uint32_t)time(NULL);
    if (minifs_write_inode((uint32_t)ino, &inode) < 0) return -EIO;
    minifs_sync_superblock();
    return 0;
}

static int mfs_statfs(const char *path, struct statvfs *stbuf)
{
    (void)path;
    memset(stbuf, 0, sizeof(*stbuf));
    stbuf->f_bsize   = MINIFS_BLOCK_SIZE;
    stbuf->f_blocks  = g_ctx.sb.total_blocks - g_ctx.sb.data_start;
    stbuf->f_bfree   = g_ctx.sb.free_blocks;
    stbuf->f_bavail  = g_ctx.sb.free_blocks;
    stbuf->f_files   = g_ctx.sb.total_inodes;
    stbuf->f_ffree   = g_ctx.sb.free_inodes;
    stbuf->f_namemax = MINIFS_NAME_MAX - 1;
    return 0;
}

/* ============================================================
 * Table des operations FUSE
 * ============================================================ */
static const struct fuse_operations mfs_ops = {
    .getattr  = mfs_getattr,
    .readdir  = mfs_readdir,
    .open     = mfs_open,
    .read     = mfs_read,
    .write    = mfs_write,
    .create   = mfs_create,
    .mkdir    = mfs_mkdir,
    .unlink   = mfs_unlink,
    .rmdir    = mfs_rmdir,
    .truncate = mfs_truncate,
    .statfs   = mfs_statfs,
};

/* ============================================================
 * Point d'entree
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr,
            "Usage : %s <image_disque> <point_de_montage> [options_fuse]\n"
            "Exemple : %s minifs.img /tmp/mnt -d\n",
            argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];

    if (minifs_open(image_path) < 0) {
        fprintf(stderr, "Erreur : impossible d'ouvrir '%s'\n", image_path);
        return EXIT_FAILURE;
    }
    printf("MiniFS monte depuis : %s\n", image_path);
    printf("  Blocs libres  : %u / %u\n",
           g_ctx.sb.free_blocks,
           g_ctx.sb.total_blocks - g_ctx.sb.data_start);
    printf("  Inodes libres : %u / %u\n",
           g_ctx.sb.free_inodes, g_ctx.sb.total_inodes);

    /* Reconstruire argv sans l'argument image */
    char **fuse_argv = malloc((size_t)(argc - 1) * sizeof(char *));
    if (!fuse_argv) { minifs_close(); return EXIT_FAILURE; }
    fuse_argv[0] = argv[0];
    for (int i = 2; i < argc; i++)
        fuse_argv[i - 1] = argv[i];

    int ret = fuse_main(argc - 1, fuse_argv, &mfs_ops, NULL);
    free(fuse_argv);
    minifs_close();
    return ret;
}

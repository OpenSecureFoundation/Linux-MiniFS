/*
 * MiniFS - Main FUSE Driver
 * Point d'entrée du système de fichiers FUSE
 */

#define FUSE_USE_VERSION 35

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../include/minifs.h"

/* Contexte global du système de fichiers */
static minifs_context_t *global_ctx = NULL;

/*
 * Wrapper pour filler
 */
struct readdir_context {
    void *buf;
    fuse_fill_dir_t filler;
};

static int readdir_filler_wrapper(void *buf, const char *name, const struct stat *stbuf, off_t off)
{
    struct readdir_context *ctx = (struct readdir_context *)buf;
    return ctx->filler(ctx->buf, name, stbuf, off, 0);
}

/*
 * Obtenir les attributs d'un fichier/répertoire
 */
static int minifs_fuse_getattr(const char *path, struct stat *stbuf,
                                struct fuse_file_info *fi)
{
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));
    
    /* Racine */
    if (strcmp(path, "/") == 0) {
        minifs_inode_t inode;
        if (minifs_read_inode(global_ctx, MINIFS_ROOT_INODE, &inode) != 0) {
            return -EIO;
        }
        
        stbuf->st_mode = inode.mode;
        stbuf->st_nlink = inode.links_count;
        stbuf->st_uid = inode.uid;
        stbuf->st_gid = inode.gid;
        stbuf->st_size = (off_t)inode.size;
        stbuf->st_atime = (time_t)inode.atime;
        stbuf->st_mtime = (time_t)inode.mtime;
        stbuf->st_ctime = (time_t)inode.ctime;
        stbuf->st_blocks = (blkcnt_t)inode.blocks_count;
        stbuf->st_blksize = MINIFS_BLOCK_SIZE;
        
        return 0;
    }
    
    /* Rechercher le fichier */
    const char *filename = strrchr(path, '/');
    if (filename) {
        filename++;
    } else {
        filename = path;
    }
    
    uint32_t parent_ino = MINIFS_ROOT_INODE;
    uint32_t ino;
    
    if (minifs_lookup(global_ctx, parent_ino, filename, &ino) != 0) {
        return -ENOENT;
    }
    
    minifs_inode_t inode;
    if (minifs_read_inode(global_ctx, ino, &inode) != 0) {
        return -EIO;
    }
    
    stbuf->st_mode = inode.mode;
    stbuf->st_nlink = inode.links_count;
    stbuf->st_uid = inode.uid;
    stbuf->st_gid = inode.gid;
    stbuf->st_size = (off_t)inode.size;
    stbuf->st_atime = (time_t)inode.atime;
    stbuf->st_mtime = (time_t)inode.mtime;
    stbuf->st_ctime = (time_t)inode.ctime;
    stbuf->st_blocks = (blkcnt_t)inode.blocks_count;
    stbuf->st_blksize = MINIFS_BLOCK_SIZE;
    
    return 0;
}

/*
 * Lire le contenu d'un répertoire
 */
static int minifs_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                                off_t offset, struct fuse_file_info *fi,
                                enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;
    
    uint32_t dir_ino = MINIFS_ROOT_INODE;
    
    /* Pour l'instant, seul le répertoire racine est supporté */
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }
    
    struct readdir_context ctx = { .buf = buf, .filler = filler };
    return minifs_readdir(global_ctx, dir_ino, &ctx, readdir_filler_wrapper);
}

/*
 * Créer un fichier
 */
static int minifs_fuse_create(const char *path, mode_t mode,
                               struct fuse_file_info *fi)
{
    (void) fi;
    
    uint32_t ino;
    int ret = minifs_create_file(global_ctx, path, mode, &ino);
    
    return ret;
}

/*
 * Lire un fichier
 */
static int minifs_fuse_read(const char *path, char *buf, size_t size, off_t offset,
                             struct fuse_file_info *fi)
{
    (void) fi;
    
    /* Rechercher l'inode */
    const char *filename = strrchr(path, '/');
    if (filename) {
        filename++;
    } else {
        filename = path;
    }
    
    uint32_t parent_ino = MINIFS_ROOT_INODE;
    uint32_t ino;
    
    if (minifs_lookup(global_ctx, parent_ino, filename, &ino) != 0) {
        return -ENOENT;
    }
    
    int bytes_read = minifs_read_file(global_ctx, ino, buf, size, offset);
    
    return bytes_read;
}

/*
 * Écrire dans un fichier
 */
static int minifs_fuse_write(const char *path, const char *buf, size_t size,
                              off_t offset, struct fuse_file_info *fi)
{
    (void) fi;
    
    /* Rechercher l'inode */
    const char *filename = strrchr(path, '/');
    if (filename) {
        filename++;
    } else {
        filename = path;
    }
    
    uint32_t parent_ino = MINIFS_ROOT_INODE;
    uint32_t ino;
    
    if (minifs_lookup(global_ctx, parent_ino, filename, &ino) != 0) {
        return -ENOENT;
    }
    
    int bytes_written = minifs_write_file(global_ctx, ino, buf, size, offset);
    
    return bytes_written;
}

/*
 * Supprimer un fichier
 */
static int minifs_fuse_unlink(const char *path)
{
    return minifs_unlink_file(global_ctx, path);
}

/*
 * Créer un répertoire
 */
static int minifs_fuse_mkdir(const char *path, mode_t mode)
{
    /* À implémenter */
    (void) path;
    (void) mode;
    return -ENOSYS;
}

/*
 * Obtenir les statistiques du système de fichiers
 */
static int minifs_fuse_statfs(const char *path, struct statvfs *stbuf)
{
    (void) path;
    
    memset(stbuf, 0, sizeof(struct statvfs));
    
    stbuf->f_bsize = MINIFS_BLOCK_SIZE;
    stbuf->f_frsize = MINIFS_BLOCK_SIZE;
    stbuf->f_blocks = global_ctx->sb.total_blocks;
    stbuf->f_bfree = global_ctx->sb.free_blocks;
    stbuf->f_bavail = global_ctx->sb.free_blocks;
    stbuf->f_files = global_ctx->sb.total_inodes;
    stbuf->f_ffree = global_ctx->sb.free_inodes;
    stbuf->f_favail = global_ctx->sb.free_inodes;
    stbuf->f_namemax = MINIFS_MAX_FILENAME;
    
    return 0;
}

/*
 * Initialisation FUSE
 */
static void *minifs_fuse_init(struct fuse_conn_info *conn,
                               struct fuse_config *cfg)
{
    (void) conn;
    cfg->kernel_cache = 1;
    
    return global_ctx;
}

/*
 * Nettoyage FUSE
 */
static void minifs_fuse_destroy(void *private_data)
{
    (void) private_data;
    
    if (global_ctx) {
        minifs_unmount(global_ctx);
    }
}

/* Table des opérations FUSE */
static struct fuse_operations minifs_oper = {
    .getattr    = minifs_fuse_getattr,
    .readdir    = minifs_fuse_readdir,
    .create     = minifs_fuse_create,
    .read       = minifs_fuse_read,
    .write      = minifs_fuse_write,
    .unlink     = minifs_fuse_unlink,
    .mkdir      = minifs_fuse_mkdir,
    .statfs     = minifs_fuse_statfs,
    .init       = minifs_fuse_init,
    .destroy    = minifs_fuse_destroy,
};

/*
 * Point d'entrée principal
 */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <device> <mountpoint> [options]\n", argv[0]);
        return 1;
    }
    
    const char *device = argv[1];
    
    /* Monter le système de fichiers */
    if (minifs_mount(device, &global_ctx) != 0) {
        fprintf(stderr, "Failed to mount filesystem\n");
        return 1;
    }
    
    printf("MiniFS mounted successfully\n");
    
    /* Ajuster les arguments pour FUSE */
    char **fuse_argv = malloc(sizeof(char *) * argc);
    fuse_argv[0] = argv[0];
    for (int i = 2; i < argc; i++) {
        fuse_argv[i - 1] = argv[i];
    }
    int fuse_argc = argc - 1;
    
    /* Lancer FUSE */
    int ret = fuse_main(fuse_argc, fuse_argv, &minifs_oper, NULL);
    
    free(fuse_argv);
    
    return ret;
}

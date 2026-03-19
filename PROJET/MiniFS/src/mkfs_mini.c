/**
 * mkfs_mini.c — Outil de formatage MiniFS
 *
 * Usage : ./mkfs_mini <image_disque> <taille_en_Mo>
 *
 * Exemple : ./mkfs_mini minifs.img 10
 *
 * Opérations réalisées :
 *  1. Créer / ouvrir le fichier image et le dimensionner
 *  2. Calculer le nombre total de blocs
 *  3. Écrire le superbloc
 *  4. Initialiser le bitmap des inodes  (tout à 0 = libre)
 *  5. Initialiser le bitmap des blocs   (tout à 0 = libre)
 *  6. Initialiser la table des inodes   (type = FREE)
 *  7. Créer le répertoire racine        (inode 0)
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "minifs.h"

/* ============================================================
 * Utilitaires bas-niveau
 * ============================================================ */

/** Écrire un bloc entier à la position (numéro_bloc) dans le fd. */
static int write_block(int fd, uint32_t block_no, const void *data)
{
    off_t offset = (off_t)block_no * MINIFS_BLOCK_SIZE;
    if (lseek(fd, offset, SEEK_SET) < 0) {
        perror("lseek");
        return -1;
    }
    if (write(fd, data, MINIFS_BLOCK_SIZE) != MINIFS_BLOCK_SIZE) {
        perror("write");
        return -1;
    }
    return 0;
}


/* ============================================================
 * Fonctions de formatage
 * ============================================================ */

/**
 * Étape 3 : Écriture du superbloc.
 */
static int write_superblock(int fd, uint32_t total_blocks)
{
    minifs_superblock_t sb;
    memset(&sb, 0, sizeof(sb));

    uint32_t data_blocks = total_blocks - MINIFS_DATA_START;

    sb.magic             = MINIFS_MAGIC;
    sb.block_size        = MINIFS_BLOCK_SIZE;
    sb.total_blocks      = total_blocks;
    sb.total_inodes      = MINIFS_MAX_INODES;
    sb.free_blocks       = data_blocks - 1; /* bloc 0 réservé à la racine */
    sb.free_inodes       = MINIFS_MAX_INODES - 1; /* inode 0 = racine    */
    sb.inode_table_start = MINIFS_INODE_TABLE_START;
    sb.data_start        = MINIFS_DATA_START;

    printf("  [SB] magic=0x%08X | total_blocks=%u | data_start=%u\n",
           sb.magic, sb.total_blocks, sb.data_start);

    return write_block(fd, MINIFS_SUPERBLOCK_BLOCK, &sb);
}

/**
 * Étape 4 : Bitmap des inodes.
 * Bit i = 1 → inode i est occupé.
 * On marque uniquement l'inode 0 (racine) comme occupé.
 */
static int write_inode_bitmap(int fd)
{
    uint8_t bitmap[MINIFS_BLOCK_SIZE];
    memset(bitmap, 0, sizeof(bitmap));

    /* Marquer l'inode 0 (racine) comme utilisé */
    bitmap[0] |= 0x01;

    printf("  [BM-I] inode 0 (racine) marqué occupé\n");
    return write_block(fd, MINIFS_INODE_BITMAP_BLOCK, bitmap);
}

/**
 * Étape 5 : Bitmap des blocs de données.
 * Bit i = 1 → bloc de données i est occupé (i relatif à data_start).
 * On marque uniquement le bloc 0 de données (répertoire racine).
 */
static int write_block_bitmap(int fd)
{
    uint8_t bitmap[MINIFS_BLOCK_SIZE];
    memset(bitmap, 0, sizeof(bitmap));

    /* Marquer le bloc de données 0 (contenu du répertoire racine) */
    bitmap[0] |= 0x01;

    printf("  [BM-B] bloc de données 0 (racine) marqué occupé\n");
    return write_block(fd, MINIFS_BLOCK_BITMAP_BLOCK, bitmap);
}

/**
 * Étape 6 : Table d'inodes.
 * Tous les inodes sont initialisés à FREE, sauf l'inode 0 (racine).
 */
static int write_inode_table(int fd)
{
    /* Préparer les 2 blocs de la table */
    uint8_t table[MINIFS_INODE_TABLE_BLOCKS][MINIFS_BLOCK_SIZE];
    memset(table, 0, sizeof(table));

    /* Construire l'inode 0 (répertoire racine) */
    minifs_inode_t *root = (minifs_inode_t *)table[0]; /* inode 0 = 1er inode du bloc 0 */

    root->type        = MINIFS_ITYPE_DIR;
    root->permissions = 0755;
    root->size        = MINIFS_BLOCK_SIZE; /* 1 bloc pour les entrées . et .. */
    root->blocks_used = 1;
    root->ctime       = (uint32_t)time(NULL);
    root->mtime       = root->ctime;
    root->atime       = root->ctime;
    root->direct[0]   = MINIFS_DATA_START; /* 1er bloc de données */

    printf("  [IT] inode 0 : DIR | perm=0755 | bloc=%u\n", root->direct[0]);

    /* Écrire les 2 blocs de la table */
    for (uint32_t i = 0; i < MINIFS_INODE_TABLE_BLOCKS; i++) {
        if (write_block(fd, MINIFS_INODE_TABLE_START + i, table[i]) < 0)
            return -1;
    }
    return 0;
}

/**
 * Étape 7 : Contenu du répertoire racine.
 * On crée les entrées "." et ".." qui pointent toutes deux sur l'inode 0.
 */
static int write_root_directory(int fd)
{
    uint8_t buf[MINIFS_BLOCK_SIZE];
    memset(buf, 0, sizeof(buf));

    minifs_dirent_t *entries = (minifs_dirent_t *)buf;

    /* Entrée "." */
    entries[0].inode = MINIFS_ROOT_INODE;
    strncpy(entries[0].name, ".", MINIFS_NAME_MAX - 1);

    /* Entrée ".." */
    entries[1].inode = MINIFS_ROOT_INODE;
    strncpy(entries[1].name, "..", MINIFS_NAME_MAX - 1);

    printf("  [ROOT] '.' → inode %u | '..' → inode %u\n",
           entries[0].inode, entries[1].inode);

    return write_block(fd, MINIFS_DATA_START, buf);
}

/* ============================================================
 * Point d'entrée
 * ============================================================ */

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage : %s <image_disque> <taille_en_Mo>\n", argv[0]);
        fprintf(stderr, "Exemple : %s minifs.img 10\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];
    long size_mb = strtol(argv[2], NULL, 10);

    if (size_mb <= 0 || size_mb > 1024) {
        fprintf(stderr, "Erreur : taille invalide (%ld Mo). Plage : 1–1024.\n", size_mb);
        return EXIT_FAILURE;
    }

    uint64_t size_bytes  = (uint64_t)size_mb * 1024 * 1024;
    uint32_t total_blocks = (uint32_t)(size_bytes / MINIFS_BLOCK_SIZE);

    if (total_blocks <= MINIFS_DATA_START + 1) {
        fprintf(stderr, "Erreur : image trop petite (minimum %d blocs).\n",
                MINIFS_DATA_START + 2);
        return EXIT_FAILURE;
    }

    printf("=== mkfs_mini ===\n");
    printf("Image      : %s\n", image_path);
    printf("Taille     : %ld Mo  (%u blocs de %d o.)\n",
           size_mb, total_blocks, MINIFS_BLOCK_SIZE);
    printf("Blocs data : %u  (blocs %u à %u)\n",
           total_blocks - MINIFS_DATA_START,
           MINIFS_DATA_START, total_blocks - 1);
    printf("\n");

    /* Ouvrir / créer l'image disque */
    int fd = open(image_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    /* Dimensionner le fichier */
    if (ftruncate(fd, (off_t)size_bytes) < 0) {
        perror("ftruncate");
        close(fd);
        return EXIT_FAILURE;
    }

    /* --- Formatage étape par étape --- */
    printf("[1/5] Superbloc...\n");
    if (write_superblock(fd, total_blocks) < 0) goto fail;

    printf("[2/5] Bitmap des inodes...\n");
    if (write_inode_bitmap(fd) < 0) goto fail;

    printf("[3/5] Bitmap des blocs...\n");
    if (write_block_bitmap(fd) < 0) goto fail;

    printf("[4/5] Table des inodes...\n");
    if (write_inode_table(fd) < 0) goto fail;

    printf("[5/5] Répertoire racine...\n");
    if (write_root_directory(fd) < 0) goto fail;

    close(fd);
    printf("\n✓ MiniFS formaté avec succès : %s\n", image_path);
    return EXIT_SUCCESS;

fail:
    fprintf(stderr, "\n✗ Erreur lors du formatage.\n");
    close(fd);
    return EXIT_FAILURE;
}

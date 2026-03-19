/**
 * minifs_core.h — Couche bas-niveau MiniFS
 *
 * Fournit toutes les primitives pour lire/écrire les structures
 * on-disk sans dépendre de FUSE. Cela permet de tester la logique
 * indépendamment du montage.
 */

#ifndef MINIFS_CORE_H
#define MINIFS_CORE_H

#include <stdint.h>
#include "minifs.h"

/* ============================================================
 * Contexte global du système de fichiers monté
 * ============================================================ */
typedef struct {
    int       fd;           /* Descripteur du fichier image              */
    minifs_superblock_t sb; /* Superbloc en mémoire (cache)              */
} minifs_ctx_t;

/* Contexte global (initialisé dans minifs_fuse.c ou les tests) */
extern minifs_ctx_t g_ctx;

/* ============================================================
 * Initialisation
 * ============================================================ */

/** Ouvrir l'image et charger le superbloc. Retourne 0 ou -errno. */
int  minifs_open(const char *path);

/** Fermer l'image et persister le superbloc. */
void minifs_close(void);

/* ============================================================
 * Blocs bas-niveau
 * ============================================================ */

/** Lire un bloc entier (MINIFS_BLOCK_SIZE octets) dans buf. */
int minifs_read_block(uint32_t block_no, void *buf);

/** Écrire un bloc entier depuis buf. */
int minifs_write_block(uint32_t block_no, const void *buf);

/* ============================================================
 * Bitmap des inodes
 * ============================================================ */

/** Retourne 1 si l'inode est occupé, 0 sinon. */
int      minifs_inode_is_used(uint32_t ino);

/** Marquer l'inode comme occupé. Met à jour free_inodes dans le SB. */
int      minifs_inode_alloc(uint32_t ino);

/** Marquer l'inode comme libre. Met à jour free_inodes dans le SB. */
int      minifs_inode_free(uint32_t ino);

/** Trouver le premier inode libre. Retourne le numéro ou -1 si plein. */
int      minifs_inode_find_free(void);

/* ============================================================
 * Bitmap des blocs de données
 * ============================================================ */

/**
 * Allouer un bloc de données libre.
 * Retourne le numéro de bloc ABSOLU (>= MINIFS_DATA_START) ou -1.
 */
int      minifs_block_alloc(void);

/** Libérer un bloc de données (numéro absolu). */
int      minifs_block_free(uint32_t block_no);

/* ============================================================
 * Inodes
 * ============================================================ */

/** Lire l'inode numéro ino dans *inode. */
int minifs_read_inode(uint32_t ino, minifs_inode_t *inode);

/** Écrire l'inode numéro ino depuis *inode. */
int minifs_write_inode(uint32_t ino, const minifs_inode_t *inode);

/* ============================================================
 * Répertoires — recherche et manipulation
 * ============================================================ */

/**
 * Résoudre un chemin absolu en numéro d'inode.
 * Retourne le numéro d'inode ou -ENOENT.
 */
int minifs_path_to_ino(const char *path);

/**
 * Chercher le nom 'name' dans le répertoire 'dir_ino'.
 * Retourne le numéro d'inode de l'entrée, ou -ENOENT.
 */
int minifs_dir_lookup(uint32_t dir_ino, const char *name);

/**
 * Ajouter l'entrée (inode, name) dans le répertoire dir_ino.
 * Retourne 0 ou -errno.
 */
int minifs_dir_add_entry(uint32_t dir_ino, uint32_t ino, const char *name);

/**
 * Supprimer l'entrée 'name' du répertoire dir_ino.
 * Retourne 0 ou -errno.
 */
int minifs_dir_remove_entry(uint32_t dir_ino, const char *name);

/* ============================================================
 * Persistance du superbloc
 * ============================================================ */

/** Écrire le superbloc en mémoire sur disque. */
int minifs_sync_superblock(void);

#endif /* MINIFS_CORE_H */

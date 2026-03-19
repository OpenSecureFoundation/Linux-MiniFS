/**
 * minifs.h — Structures on-disk de MiniFS
 *
 * Organisation du disque (image ou partition) :
 *
 *  +------------------+  offset 0
 *  |   Superbloc      |  1 bloc  (MINIFS_BLOCK_SIZE)
 *  +------------------+  offset 1 bloc
 *  |  Bitmap inodes   |  1 bloc
 *  +------------------+  offset 2 blocs
 *  |  Bitmap blocs    |  1 bloc
 *  +------------------+  offset 3 blocs
 *  |  Table d'inodes  |  MINIFS_INODE_TABLE_BLOCKS blocs
 *  +------------------+  offset (3 + MINIFS_INODE_TABLE_BLOCKS) blocs
 *  |   Blocs données  |  reste du disque
 *  +------------------+
 */

#ifndef MINIFS_H
#define MINIFS_H

#include <stdint.h>

/* ============================================================
 * Constantes globales
 * ============================================================ */

#define MINIFS_MAGIC          0x4D494E49   /* "MINI" en ASCII */
#define MINIFS_BLOCK_SIZE     4096         /* Taille d'un bloc en octets     */
#define MINIFS_MAX_INODES     128          /* Nombre maximal d'inodes        */
#define MINIFS_INODE_SIZE     64           /* Taille d'un inode en octets    */
#define MINIFS_NAME_MAX       28           /* Longueur max d'un nom de fich. */

/* Nombre de blocs pour la table d'inodes :
 * 128 inodes × 64 octets = 8192 octets = 2 blocs */
#define MINIFS_INODE_TABLE_BLOCKS  \
    ((MINIFS_MAX_INODES * MINIFS_INODE_SIZE) / MINIFS_BLOCK_SIZE)

/* Offsets des zones (en numéro de bloc absolu) */
#define MINIFS_SUPERBLOCK_BLOCK     0
#define MINIFS_INODE_BITMAP_BLOCK   1
#define MINIFS_BLOCK_BITMAP_BLOCK   2
#define MINIFS_INODE_TABLE_START    3
#define MINIFS_DATA_START           (3 + MINIFS_INODE_TABLE_BLOCKS) /* = 5 */

/* Nombre maximum de blocs directs dans un inode */
#define MINIFS_DIRECT_BLOCKS  10

/* Numéro d'inode de la racine */
#define MINIFS_ROOT_INODE     0

/* Types d'inode */
#define MINIFS_ITYPE_FREE     0   /* Inode libre               */
#define MINIFS_ITYPE_FILE     1   /* Fichier régulier          */
#define MINIFS_ITYPE_DIR      2   /* Répertoire                */

/* ============================================================
 * Structure du Superbloc  (doit tenir dans 1 bloc = 4096 o.)
 * ============================================================ */
typedef struct {
    uint32_t  magic;             /* Signature : MINIFS_MAGIC               */
    uint32_t  block_size;        /* Taille d'un bloc (octets)              */
    uint32_t  total_blocks;      /* Nombre total de blocs sur le disque    */
    uint32_t  total_inodes;      /* Nombre total d'inodes                  */
    uint32_t  free_blocks;       /* Blocs de données libres                */
    uint32_t  free_inodes;       /* Inodes libres                          */
    uint32_t  inode_table_start; /* Numéro du 1er bloc de la table inodes  */
    uint32_t  data_start;        /* Numéro du 1er bloc de données          */
    uint8_t   _padding[4096 - 8 * sizeof(uint32_t)]; /* Rembourrage        */
} __attribute__((packed)) minifs_superblock_t;

/* ============================================================
 * Structure d'un Inode  (64 octets fixes)
 * ============================================================ */
typedef struct {
    uint8_t   type;              /* MINIFS_ITYPE_*                         */
    uint8_t   _reserved;         /* Alignement                             */
    uint16_t  permissions;       /* Permissions Unix (ex. 0644, 0755)      */
    uint32_t  size;              /* Taille du fichier en octets            */
    uint32_t  blocks_used;       /* Nombre de blocs alloués                */
    uint32_t  ctime;             /* Date de création   (timestamp Unix)    */
    uint32_t  mtime;             /* Date de modif.     (timestamp Unix)    */
    uint32_t  atime;             /* Date d'accès       (timestamp Unix)    */
    uint32_t  direct[MINIFS_DIRECT_BLOCKS]; /* Blocs directs (0 = libre)   */
    /* Total : 1+1+2+4+4+4+4+4 + 10×4 = 64 octets ✓ */
} __attribute__((packed)) minifs_inode_t;

/* ============================================================
 * Structure d'une entrée de répertoire
 * ============================================================ */
typedef struct {
    uint32_t  inode;             /* Numéro d'inode (0 = entrée libre)      */
    char      name[MINIFS_NAME_MAX]; /* Nom du fichier / répertoire        */
    /* Total : 4 + 28 = 32 octets  →  128 entrées par bloc de 4096 o.    */
} __attribute__((packed)) minifs_dirent_t;

/* Entrées de répertoire par bloc */
#define MINIFS_DIRENTS_PER_BLOCK \
    (MINIFS_BLOCK_SIZE / sizeof(minifs_dirent_t))

/* ============================================================
 * Vérifications statiques (compilateur)
 * ============================================================ */
_Static_assert(sizeof(minifs_superblock_t) == MINIFS_BLOCK_SIZE,
               "Superbloc doit faire exactement MINIFS_BLOCK_SIZE octets");
_Static_assert(sizeof(minifs_inode_t) == MINIFS_INODE_SIZE,
               "Inode doit faire exactement MINIFS_INODE_SIZE octets");
_Static_assert(sizeof(minifs_dirent_t) == 32,
               "Entrée de répertoire doit faire 32 octets");

#endif /* MINIFS_H */

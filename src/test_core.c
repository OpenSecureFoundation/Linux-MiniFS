/**
 * test_core.c — Tests unitaires de la couche minifs_core
 *
 * Teste sans FUSE :
 *  - Ouverture de l'image et vérification du superbloc
 *  - Allocation / libération d'inodes
 *  - Allocation / libération de blocs
 *  - Lecture / écriture d'un inode
 *  - Création d'un fichier via dir_add_entry
 *  - Résolution de chemin (path_to_ino)
 *  - Lecture du contenu du répertoire racine
 *  - Suppression d'une entrée de répertoire
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "minifs.h"
#include "minifs_core.h"

/* ============================================================
 * Macro de test
 * ============================================================ */
static int g_passed = 0;
static int g_failed = 0;

#define TEST(label, cond) do {                                  \
    if (cond) {                                                  \
        printf("  [PASS] %s\n", label);                         \
        g_passed++;                                              \
    } else {                                                     \
        printf("  [FAIL] %s  (ligne %d)\n", label, __LINE__);   \
        g_failed++;                                              \
    }                                                            \
} while(0)

/* ============================================================
 * Tests
 * ============================================================ */

static void test_open_and_superblock(const char *path)
{
    printf("\n--- 1. Ouverture et superbloc ---\n");
    int r = minifs_open(path);
    TEST("minifs_open() retourne 0", r == 0);
    TEST("magic valide",       g_ctx.sb.magic        == MINIFS_MAGIC);
    TEST("block_size valide",  g_ctx.sb.block_size   == MINIFS_BLOCK_SIZE);
    TEST("total_inodes = 128", g_ctx.sb.total_inodes == MINIFS_MAX_INODES);
    TEST("free_inodes  = 127", g_ctx.sb.free_inodes  == 127);
    TEST("data_start   = 5",   g_ctx.sb.data_start   == MINIFS_DATA_START);
}

static void test_inode_bitmap(void)
{
    printf("\n--- 2. Bitmap des inodes ---\n");
    TEST("inode 0 est occupe",  minifs_inode_is_used(0) == 1);
    TEST("inode 1 est libre",   minifs_inode_is_used(1) == 0);
    TEST("find_free trouve 1",  minifs_inode_find_free() == 1);

    /* Allouer l'inode 1 */
    int r = minifs_inode_alloc(1);
    TEST("alloc inode 1 OK",    r == 0);
    TEST("inode 1 occupe apres alloc", minifs_inode_is_used(1) == 1);
    TEST("free_inodes decremente",     g_ctx.sb.free_inodes == 126);
    TEST("find_free trouve 2",         minifs_inode_find_free() == 2);

    /* Libérer l'inode 1 */
    r = minifs_inode_free(1);
    TEST("free inode 1 OK",     r == 0);
    TEST("inode 1 libre apres free",   minifs_inode_is_used(1) == 0);
    TEST("free_inodes restaure",       g_ctx.sb.free_inodes == 127);
}

static void test_block_bitmap(void)
{
    printf("\n--- 3. Bitmap des blocs ---\n");
    uint32_t free_avant = g_ctx.sb.free_blocks;

    int b1 = minifs_block_alloc();
    TEST("alloc bloc b1 >= data_start", b1 >= (int)MINIFS_DATA_START);
    TEST("free_blocks decremente",      g_ctx.sb.free_blocks == free_avant - 1);

    int b2 = minifs_block_alloc();
    TEST("alloc bloc b2 different de b1", b2 != b1);
    TEST("free_blocks decremente 2",      g_ctx.sb.free_blocks == free_avant - 2);

    int r = minifs_block_free((uint32_t)b1);
    TEST("free bloc b1 OK",              r == 0);
    TEST("free_blocks restaure de 1",    g_ctx.sb.free_blocks == free_avant - 1);

    /* b1 doit être réutilisé en premier (first-fit) */
    int b3 = minifs_block_alloc();
    TEST("b1 realloc en premier",        b3 == b1);

    /* Nettoyer */
    minifs_block_free((uint32_t)b2);
    minifs_block_free((uint32_t)b3);
    TEST("free_blocks restaure complet", g_ctx.sb.free_blocks == free_avant);
}

static void test_inode_readwrite(void)
{
    printf("\n--- 4. Lecture / ecriture d'un inode ---\n");

    /* Lire l'inode 0 (racine) */
    minifs_inode_t inode;
    int r = minifs_read_inode(0, &inode);
    TEST("read_inode(0) OK",       r == 0);
    TEST("inode 0 est DIR",        inode.type == MINIFS_ITYPE_DIR);
    TEST("inode 0 perm = 0755",    inode.permissions == 0755);
    TEST("inode 0 direct[0] = 5",  inode.direct[0] == MINIFS_DATA_START);

    /* Écrire et relire un inode temporaire (inode 5) */
    minifs_inode_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.type        = MINIFS_ITYPE_FILE;
    tmp.permissions = 0644;
    tmp.size        = 1234;
    tmp.ctime       = 999999;

    r = minifs_write_inode(5, &tmp);
    TEST("write_inode(5) OK", r == 0);

    minifs_inode_t check;
    r = minifs_read_inode(5, &check);
    TEST("read_inode(5) apres write OK", r == 0);
    TEST("type persisté",                check.type        == MINIFS_ITYPE_FILE);
    TEST("size persistée",               check.size        == 1234);
    TEST("ctime persisté",               check.ctime       == 999999);
    TEST("permissions persistées",       check.permissions == 0644);

    /* Remettre l'inode 5 à zéro (nettoyage) */
    memset(&tmp, 0, sizeof(tmp));
    minifs_write_inode(5, &tmp);
}

static void test_directory_operations(void)
{
    printf("\n--- 5. Operations sur les repertoires ---\n");

    /* Chercher "." dans la racine */
    int r = minifs_dir_lookup(MINIFS_ROOT_INODE, ".");
    TEST("lookup '.' dans racine retourne 0",  r == MINIFS_ROOT_INODE);

    r = minifs_dir_lookup(MINIFS_ROOT_INODE, "..");
    TEST("lookup '..' dans racine retourne 0", r == MINIFS_ROOT_INODE);

    r = minifs_dir_lookup(MINIFS_ROOT_INODE, "inexistant");
    TEST("lookup inexistant retourne -ENOENT",  r == -ENOENT);

    /* Ajouter une entrée de test */
    r = minifs_dir_add_entry(MINIFS_ROOT_INODE, 7, "testfile.txt");
    TEST("dir_add_entry OK", r == 0);

    r = minifs_dir_lookup(MINIFS_ROOT_INODE, "testfile.txt");
    TEST("lookup 'testfile.txt' retourne 7", r == 7);

    /* Supprimer l'entrée */
    r = minifs_dir_remove_entry(MINIFS_ROOT_INODE, "testfile.txt");
    TEST("dir_remove_entry OK", r == 0);

    r = minifs_dir_lookup(MINIFS_ROOT_INODE, "testfile.txt");
    TEST("lookup apres suppression = -ENOENT", r == -ENOENT);
}

static void test_path_resolution(void)
{
    printf("\n--- 6. Resolution de chemin ---\n");

    /* "/" doit pointer sur l'inode 0 */
    int r = minifs_path_to_ino("/");
    TEST("path '/' = inode 0", r == MINIFS_ROOT_INODE);

    /* Créer un sous-répertoire fictif pour le test */
    /* On alloue manuellement l'inode 2 comme répertoire "docs" */
    int new_ino = minifs_inode_find_free();
    TEST("find_free = 1", new_ino == 1);

    /* Ajouter l'entrée "docs" dans la racine */
    minifs_inode_alloc((uint32_t)new_ino);
    minifs_inode_t dir;
    memset(&dir, 0, sizeof(dir));
    dir.type = MINIFS_ITYPE_DIR;
    dir.permissions = 0755;
    int blk = minifs_block_alloc();
    dir.direct[0]   = (uint32_t)blk;
    dir.blocks_used = 1;
    dir.size        = MINIFS_BLOCK_SIZE;
    minifs_write_inode((uint32_t)new_ino, &dir);
    minifs_dir_add_entry(MINIFS_ROOT_INODE, (uint32_t)new_ino, "docs");

    r = minifs_path_to_ino("/docs");
    TEST("path '/docs' trouve l'inode", r == new_ino);

    /* Nettoyage */
    minifs_dir_remove_entry(MINIFS_ROOT_INODE, "docs");
    minifs_block_free((uint32_t)blk);
    minifs_inode_free((uint32_t)new_ino);

    r = minifs_path_to_ino("/docs");
    TEST("path '/docs' apres suppression = -ENOENT", r == -ENOENT);
}

static void test_sync(void)
{
    printf("\n--- 7. Persistance (sync) ---\n");
    uint32_t fb_avant = g_ctx.sb.free_blocks;
    g_ctx.sb.free_blocks = 42; /* modifier en mémoire */
    int r = minifs_sync_superblock();
    TEST("sync_superblock OK", r == 0);

    /* Relire le superbloc depuis le disque pour vérifier */
    minifs_superblock_t sb2;
    minifs_read_block(MINIFS_SUPERBLOCK_BLOCK, &sb2);
    TEST("valeur persistee sur disque", sb2.free_blocks == 42);

    /* Remettre la valeur initiale */
    g_ctx.sb.free_blocks = fb_avant;
    minifs_sync_superblock();
}

/* ============================================================
 * Point d'entree des tests
 * ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage : %s <image_minifs>\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("=== Tests unitaires minifs_core ===\n");
    printf("Image : %s\n", argv[1]);

    test_open_and_superblock(argv[1]);
    test_inode_bitmap();
    test_block_bitmap();
    test_inode_readwrite();
    test_directory_operations();
    test_path_resolution();
    test_sync();

    minifs_close();

    printf("\n=== Résultat : %d/%d tests passés ===\n",
           g_passed, g_passed + g_failed);

    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

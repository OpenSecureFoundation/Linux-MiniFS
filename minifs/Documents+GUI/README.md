# MiniFS - Système de Fichiers Minimal Éducatif

## Table des matières

- [Introduction](#introduction)
- [Caractéristiques](#caractéristiques)
- [Architecture](#architecture)
- [Installation](#installation)
- [Utilisation](#utilisation)
- [Exemples](#exemples)
- [Développement](#développement)
- [Dépannage](#dépannage)
- [Contribution](#contribution)
- [Licence](#licence)

## Introduction

MiniFS est un système de fichiers minimal implémenté avec FUSE (Filesystem in Userspace) à des fins éducatives. Il illustre les concepts fondamentaux de la gestion de fichiers dans les systèmes d'exploitation modernes.

### Pourquoi MiniFS ?

-  Éducatif : Code simple et bien documenté
-  Sécurisé : Fonctionne en espace utilisateur (pas de risque kernel panic)
-  Portable : Compatible avec toutes les distributions Linux
-  Fonctionnel : Toutes les opérations de base implémentées

## Caractéristiques

### Fonctionnalités implémentées

- [x] Gestion des inodes (allocation, libération, lecture, écriture)
- [x] Allocation dynamique de blocs avec bitmap
- [x] Support hiérarchique des répertoires
- [x] Opérations de lecture/écriture sur les fichiers
- [x] Superbloc avec métadonnées du système
- [x] Formatage de partition/image disque
- [x] Détection basique de la fragmentation
- [x] Gestion des permissions POSIX
- [x] Timestamps (atime, mtime, ctime)
- [x] Support des liens durs (hard links)

### Spécifications techniques

| Paramètre                 | Valeur                            |
|-----------                |--------                           |
| Taille de bloc            | 4096 octets                       |
| Taille max fichier        | ~50 MB (avec indirection simple)  |
| Longueur max nom          | 255 caractères                    |
| Nombre max inodes         | Configurable (défaut: 4096)       |
| Format on-disk            | Little-endian                     |

## Architecture

### Structure du système de fichiers de MiniFS

```
┌─────────────────────────────────────────────────────────┐
│                     SUPERBLOC (Bloc 0)                  │
│  - Magic number, version, état                          │
│  - Statistiques (blocs/inodes libres)                   │
│  - Pointeurs vers structures                            │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│               BITMAP INODES (Bloc 1)                    │
│  Un bit par inode (0=libre, 1=occupé)                   │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│          TABLE DES INODES (Blocs 2-N)                   │
│  Métadonnées des fichiers et répertoires                │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│               BITMAP BLOCS (Blocs N+1-M)                │
│  Un bit par bloc de données                             │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│            BLOCS DE DONNÉES (Blocs M+1-FIN)             │
│  Contenu réel des fichiers et répertoires               │
└─────────────────────────────────────────────────────────┘
```

### Composants logiciels de MiniFS

```
┌──────────────────────────────────────────┐
│        Applications Utilisateur          │
└───────────────┬──────────────────────────┘
                │ Appels système (open, read, write, ...)
┌───────────────▼──────────────────────────┐
│              Noyau Linux VFS             │
└───────────────┬──────────────────────────┘
                │ Interface FUSE
┌───────────────▼──────────────────────────┐
│          Bibliothèque libfuse            │
└───────────────┬──────────────────────────┘
                │ Callbacks
┌───────────────▼──────────────────────────┐
│           MiniFS (minifs_fuse)           │
│  ┌────────────────────────────────────┐  │
│  │  Gestion inodes                    │  │
│  │  Allocation blocs                  │  │
│  │  Opérations répertoires            │  │
│  │  Lecture/Écriture fichiers         │  │
│  └────────────────────────────────────┘  │
└───────────────┬──────────────────────────┘
                │ I/O direct
┌───────────────▼──────────────────────────┐
│      Image disque / Périphérique bloc    │
└──────────────────────────────────────────┘
```

## Installation

### Prérequis

Système d'exploitation : OpenSUSE Tumbleweed (ou toute distribution Linux récente)

Paquets requis :
```invite de commande : 
sudo zypper install -y \
    gcc \
    make \
    fuse3 \
    fuse3-devel \
    pkg-config \
    git
```

### Installation automatique

```invite de commande : 
# Cloner ou extraire le projet
cd minifs-fuse

# Compiler et installer
sudo ./scripts/install.sh
```

### Installation manuelle

```invite de commande : 
# Compiler
make clean
make

# Installer les binaires
sudo make install

# Vérifier l'installation
mkfs.minifs --version
```

## Utilisation

### 1. Créer une image disque

```invite de commande : 
# Créer une image de 100 MB
dd if=/dev/zero of=disk.img bs=1M count=100

# Formater avec MiniFS
mkfs.minifs disk.img
```

Sortie attendue :
```
mkfs.minifs 1.0.0

Formatting disk.img...
Block size: 4096 bytes
Total blocks: 25600 (100.0 MB)
Inode count: 4096
Blocks per inode: 6

Layout:
  Superblock:      1 block   (0)
  Inode bitmap:    1 block   (1)
  Inode table:     256 blocks (2-257)
  Block bitmap:    7 blocks  (258-264)
  Data blocks:     25335 blocks (265-25599)

Available space: 99.0 MB
Root directory created.

Filesystem created successfully.
```

### 2. Monter le système de fichiers

```invite de commande : 
# Créer le point de montage
mkdir -p /mnt/minifs

# Monter
sudo mount.minifs disk.img /mnt/minifs

# Vérifier
df -h /mnt/minifs
```

### 3. Utiliser le système de fichiers

```invite de commande : 
# Créer des fichiers
echo "Hello MiniFS!" > /mnt/minifs/test.txt

# Créer des répertoires
mkdir /mnt/minifs/documents
mkdir /mnt/minifs/documents/photos

# Copier des fichiers
cp mon_fichier.pdf /mnt/minifs/documents/

# Lister le contenu
ls -la /mnt/minifs/

# Lire un fichier
cat /mnt/minifs/test.txt
```

### 4. Démonter

```invite de commande : 
# Démonter proprement
sudo umount /mnt/minifs

# Ou utiliser l'outil dédié
sudo umount.minifs /mnt/minifs
```

### 5. Vérifier le système de fichiers

```invite de commande : 
# Vérifier l'intégrité
sudo fsck.minifs disk.img

# Afficher les statistiques
sudo debugfs.minifs disk.img --stats

# Analyser la fragmentation
sudo debugfs.minifs disk.img --fragmentation
```

## Exemples

### Exemple 1 : Créer et utiliser un disque de 1 GB

```invite de commande : 
#!/bin/invite de commande : 

# Créer l'image
dd if=/dev/zero of=bigdisk.img bs=1M count=1024

# Formater
mkfs.minifs bigdisk.img --inodes 8192

# Monter
mkdir -p /mnt/bigdisk
sudo mount.minifs bigdisk.img /mnt/bigdisk

# Utiliser
cd /mnt/bigdisk
mkdir -p projects/{src,include,docs}
echo "# Mon Projet" > projects/README.md

# Nettoyer
cd ~
sudo umount /mnt/bigdisk
```

### Exemple 2 : Test de performance

```invite de commande : 
# Créer 1000 fichiers
for i in {1..1000}; do
    echo "File $i" > /mnt/minifs/file_$i.txt
done

# Mesurer le temps de lecture
time cat /mnt/minifs/file_*.txt > /dev/null

# Analyser la fragmentation
sudo debugfs.minifs disk.img --fragmentation
```

### Exemple 3 : Sauvegarde et restauration

```invite de commande : 
# Sauvegarder
sudo umount /mnt/minifs
cp disk.img disk_backup.img

# Restaurer en cas de problème
cp disk_backup.img disk.img
sudo mount.minifs disk.img /mnt/minifs
```

## Développement

### Structure du code source

```
minifs-fuse/
├── src/
│   ├── main.c              # Point d'entrée FUSE
│   ├── filesystem.c        # Formatage et montage
│   ├── inode.c             # Gestion des inodes
│   ├── block.c             # Allocation de blocs
│   ├── directory.c         # Opérations sur répertoires
│   ├── file_ops.c          # Opérations sur fichiers
│   ├── utils.c             # Utilitaires (bitmap, etc.)
│   └── debug.c             # Outils de débogage
├── include/
│   └── minifs.h            # Structures de données
├── tools/
│   ├── mkfs.c              # Outil de formatage
│   ├── fsck.c              # Vérificateur de système
│   ├── debugfs.c           # Explorateur/débogueur
│   └── mount_helper.c      # Assistant de montage
├── tests/
│   └── test_suite.sh       # Suite de tests
├── docs/
│   ├── ARCHITECTURE.md     # Documentation architecture
│   ├── API.md              # Documentation API
│   └── TROUBLESHOOTING.md  # Guide de dépannage
└── scripts/
    ├── install.sh          # Script d'installation
    ├── uninstall.sh        # Script de désinstallation
    └── test.sh             # Script de test
```

### Compiler en mode développement

```invite de commande : 
# Compilation avec symboles de débogage
make clean
make DEBUG=1

# Exécuter avec valgrind (détection de fuites mémoire)
valgrind --leak-check=full ./mount.minifs disk.img /mnt/minifs
```

### IDE et outils recommandés

IDE recommandés :
- VSCode avec extensions :
  - C/C++ (Microsoft)
  - C/C++ Extension Pack
  - Makefile Tools
- CLion (JetBrains) - version complète
- Code::Blocks - léger et gratuit

Outils de développement :
```invite de commande : 
# Installer les outils de développement
sudo zypper install -y \
    gdb \              # Débogueur
    valgrind \         # Détection de fuites mémoire
    strace \           # Traçage d'appels système
    ltrace \           # Traçage d'appels de bibliothèque
    cppcheck \         # Analyse statique
    clang-format       # Formatage de code
```

Configuration VSCode recommandée :
```json
{
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/include",
        "/usr/include/fuse3"
    ],
    "C_Cpp.default.defines": [
        "FUSE_USE_VERSION=35"
    ],
    "C_Cpp.default.compilerPath": "/usr/bin/gcc"
}
```

### Ajouter une nouvelle fonctionnalité

Exemple : Ajouter le support des liens symboliques

1. Modifier la structure (`include/minifs.h`) :
```c
#define MINIFS_FT_SYMLINK 7

int minifs_symlink(minifs_context_t *ctx, const char *target, 
                   const char *linkpath);
```

2. Implémenter la fonction (`src/file_ops.c`) :
```c
int minifs_symlink(minifs_context_t *ctx, const char *target,
                   const char *linkpath) {
    // Implémentation
}
```

3. Ajouter le callback FUSE (`src/main.c`) :
```c
static struct fuse_operations minifs_ops = {
    // ... autres opérations
    .symlink = minifs_fuse_symlink,
};
```

4. Recompiler et tester :
```invite de commande : 
make clean && make
sudo make install
# Tester la nouvelle fonctionnalité
```

### Tests et validation

```invite de commande : 
# Suite de tests complète
./scripts/test.sh

# Tests unitaires spécifiques
./tests/test_inodes.sh
./tests/test_blocks.sh
./tests/test_directories.sh

# Test de robustesse (crash recovery)
./tests/test_crash.sh
```

## Dépannage

Voir le fichier [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) pour le guide complet.

### Problèmes courants

Erreur : "fuse: device not found"
```invite de commande : 
# Charger le module FUSE
sudo modprobe fuse

# Vérifier
lsmod | grep fuse
```

Erreur : "Transport endpoint is not connected"
```invite de commande : 
# Forcer le démontage
sudo fusermount3 -u /mnt/minifs

# Ou
sudo umount -l /mnt/minifs
```

Performance dégradée
```invite de commande : 
# Vérifier la fragmentation
sudo debugfs.minifs disk.img --fragmentation

# Si fragmentation > 30%, envisager la défragmentation
# (fonctionnalité à implémenter)
```

## Performances

### Benchmarks typiques

Sur une VM OpenSUSE avec 2 CPU cores et 4 GB RAM :

| Opération                                 | Performance |
|-----------                                |-------------|
| Création fichier                          | ~0.5 ms     |
| Lecture séquentielle                      | 80-120 MB/s |
| Écriture séquentielle                     | 60-90 MB/s  |
| Recherche dans répertoire (100 fichiers)  | ~10 ms      |
| Recherche dans répertoire (1000 fichiers) | ~80 ms      |

## Contribution

Les contributions sont les bienvenues ! Suivez ces étapes :

1. Fork le projet
2. Créer une branche (`git checkout -b feature/AmazingFeature`)
3. Commit les changements (`git commit -m 'Add AmazingFeature'`)
4. Push sur la branche (`git push origin feature/AmazingFeature`)
5. Ouvrir une Pull Request

## Licence

Ce projet est sous licence MIT. Voir le fichier `LICENSE` pour plus de détails.

## Auteurs

- Projet MiniFS - Implémentation éducative

## Remerciements

- L'équipe FUSE pour la bibliothèque libfuse
- La communauté Linux pour la documentation
- Les étudiants et enseignants qui utilisent ce projet

## Ressources supplémentaires

- [Documentation FUSE](https://libfuse.github.io/doxygen/)
- [Linux VFS Documentation](https://www.kernel.org/doc/html/latest/filesystems/vfs.html)
- [File System Design](https://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf)

---

Version : 1.0.0  
Dernière mise à jour : Mars 2025  
Support : Pour toute question, créez une issue sur le dépôt du projet

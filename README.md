# Conception et développement de MiniFS, un type de système de fichiers minimal: cas de Open Suse

# Objectifs:
1. Gestion des inodes
2. Allocation de blocs
3. Répertoires + fichiers
4. Lecture / écriture
5. Superbloc minimal


# Fonctionnalités attendues
1. Support fichiers & dossiers
2. Formatage de partition
3. Structure : superbloc, table inodes, table blocs, data blocks
4. Détection fragmentation simple

## Introduction

MiniFS est un système de fichiers minimal implémenté avec FUSE (Filesystem in Userspace) à des fins éducatives. Il illustre les concepts fondamentaux de la gestion de fichiers dans les systèmes d'exploitation modernes.

##  Caractéristiques

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

| Paramètre | Valeur |
|-----------|--------|
| Taille de bloc | 4096 octets |
| Taille max fichier | ~50 MB (avec indirection simple) |
| Longueur max nom | 255 caractères |
| Nombre max inodes | Configurable (défaut: 4096) |
| Format on-disk | Little-endian |

## Architecture

### Structure du système de fichiers

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

### Composants logiciels

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

##  Installation

### Prérequis

**Système d'exploitation :** OpenSUSE Tumbleweed (ou toute distribution Linux récente)

**Paquets requis :**
```bash
sudo zypper install -y \
    gcc \
    make \
    fuse3 \
    fuse3-devel \
    pkg-config \
    git
```

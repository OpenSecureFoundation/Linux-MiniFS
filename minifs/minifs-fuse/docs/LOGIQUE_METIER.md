# Logique Métier MiniFS

## Table des matières

1. [Vue d'ensemble](#vue-densemble)
2. [Gestion du Superbloc](#gestion-du-superbloc)
3. [Gestion des Inodes](#gestion-des-inodes)
4. [Allocation de Blocs](#allocation-de-blocs)
5. [Opérations sur Répertoires](#opérations-sur-répertoires)
6. [Opérations sur Fichiers](#opérations-sur-fichiers)
7. [Gestion de la Fragmentation](#gestion-de-la-fragmentation)
8. [Algorithmes Clés](#algorithmes-clés)

---

## Vue d'ensemble

MiniFS implémente une logique métier complète pour la gestion d'un système de fichiers minimal. L'architecture est basée sur quatre composants principaux qui interagissent pour fournir toutes les fonctionnalités d'un système de fichiers.

### Principes de conception

1. **Simplicité** : Code clair et facile à comprendre
2. **Sécurité** : Validation stricte des entrées
3. **Cohérence** : Maintien de l'intégrité des données
4. **Performance** : Optimisation des opérations critiques

---

## Gestion du Superbloc

### Responsabilités

Le superbloc est la structure centrale contenant toutes les métadonnées globales du système de fichiers.

### Opérations

#### 1. Lecture du Superbloc

```
Fonction: read_superblock(context)

Entrées:
  - context: Contexte du système de fichiers

Sorties:
  - Superbloc chargé en mémoire
  - Code d'erreur si échec

Logique:
  1. Lire le bloc 0 depuis le disque
  2. Vérifier le magic number (0x4D494E46)
  3. Valider la version
  4. Vérifier l'état du système (VALID_FS / ERROR_FS)
  5. Charger les métadonnées en mémoire
  6. Si état ERROR_FS:
     - Journaliser l'avertissement
     - Recommander fsck
  7. Retourner succès ou erreur

Complexité: O(1)
```

#### 2. Écriture du Superbloc

```
Fonction: write_superblock(context)

Entrées:
  - context: Contexte avec superbloc modifié

Sorties:
  - Succès ou erreur

Logique:
  1. Mettre à jour write_time avec timestamp actuel
  2. Incrémenter mount_count si montage
  3. Valider la cohérence des compteurs:
     - free_blocks <= total_blocks
     - free_inodes <= total_inodes
  4. Écrire le bloc 0 sur le disque
  5. Synchroniser (fsync)
  6. Retourner succès

Complexité: O(1)
```

#### 3. Validation du Superbloc

```
Fonction: validate_superblock(sb)

Entrées:
  - sb: Structure superbloc

Sorties:
  - Booléen (valide / invalide)

Logique:
  1. Vérifier magic_number == 0x4D494E46
  2. Vérifier version == 1
  3. Vérifier block_size == 4096
  4. Vérifier total_blocks > 0
  5. Vérifier total_inodes > 0
  6. Vérifier free_blocks <= total_blocks
  7. Vérifier free_inodes <= total_inodes
  8. Vérifier inode_table_block > 0
  9. Vérifier data_block_start > inode_table_block
  10. Retourner true si toutes validations passent

Complexité: O(1)
```

---

## Gestion des Inodes

### Responsabilités

Les inodes stockent les métadonnées de chaque fichier et répertoire.

### Opérations

#### 1. Allocation d'Inode

```
Fonction: alloc_inode(context)

Entrées:
  - context: Contexte du système de fichiers

Sorties:
  - Numéro d'inode alloué
  - 0 si échec

Logique:
  1. Acquérir le verrou (thread-safety)
  2. Vérifier free_inodes > 0
     - Si = 0: retourner erreur ENOSPC
  3. Rechercher premier bit libre dans inode_bitmap:
     - find_first_zero_bit(inode_bitmap, total_inodes)
  4. Marquer le bit comme occupé:
     - set_bit(inode_bitmap, inode_num)
  5. Écrire le bitmap modifié sur disque
  6. Décrémenter free_inodes dans superbloc
  7. Écrire le superbloc
  8. Libérer le verrou
  9. Retourner inode_num

Complexité: O(n) dans le pire cas pour find_first_zero_bit
Optimisation: Parcours par octets (skip 0xFF)
```

#### 2. Libération d'Inode

```
Fonction: free_inode(context, inode_num)

Entrées:
  - context: Contexte
  - inode_num: Numéro d'inode à libérer

Sorties:
  - Succès ou erreur

Logique:
  1. Acquérir le verrou
  2. Valider inode_num:
     - Doit être > 0 et < total_inodes
     - Ne doit pas être ROOT_INODE (1)
  3. Lire l'inode depuis le disque
  4. Si links_count > 0:
     - Journaliser avertissement
     - Retourner erreur
  5. Libérer tous les blocs de données:
     - Pour chaque bloc direct non-nul:
       - free_block(context, block_num)
     - Si indirect_block != 0:
       - Lire le bloc d'indirection
       - Libérer chaque bloc référencé
       - Libérer le bloc d'indirection
  6. Marquer le bit comme libre:
     - clear_bit(inode_bitmap, inode_num)
  7. Écrire le bitmap
  8. Incrémenter free_inodes
  9. Écrire le superbloc
  10. Libérer le verrou
  11. Retourner succès

Complexité: O(n) où n = nombre de blocs du fichier
```

#### 3. Lecture d'Inode

```
Fonction: read_inode(context, inode_num, inode_out)

Entrées:
  - context: Contexte
  - inode_num: Numéro d'inode
  - inode_out: Pointeur vers structure inode

Sorties:
  - Inode rempli
  - Code d'erreur si échec

Logique:
  1. Valider inode_num (> 0, < total_inodes)
  2. Calculer la position sur le disque:
     - inode_size = sizeof(minifs_inode_t)  // 256 bytes
     - inodes_per_block = block_size / inode_size  // 16
     - block_index = inode_num / inodes_per_block
     - block_offset = inode_num % inodes_per_block
     - disk_block = inode_table_block + block_index
  3. Lire le bloc depuis le disque
  4. Copier l'inode depuis le bloc vers inode_out
  5. Retourner succès

Complexité: O(1)
```

#### 4. Écriture d'Inode

```
Fonction: write_inode(context, inode_num, inode)

Entrées:
  - context: Contexte
  - inode_num: Numéro d'inode
  - inode: Structure inode à écrire

Sorties:
  - Succès ou erreur

Logique:
  1. Valider inode_num
  2. Calculer la position (même formule que lecture)
  3. Lire le bloc contenant l'inode
  4. Modifier l'inode dans le bloc
  5. Écrire le bloc modifié sur disque
  6. Retourner succès

Complexité: O(1)
```

#### 5. Obtention d'un Bloc de Données

```
Fonction: get_block(context, inode, block_index, block_num_out)

Entrées:
  - context: Contexte
  - inode: Structure inode
  - block_index: Index du bloc (0, 1, 2, ...)
  - block_num_out: Pointeur pour stocker le numéro de bloc

Sorties:
  - Numéro de bloc physique
  - 0 si le bloc n'existe pas

Logique:
  1. Si block_index < DIRECT_BLOCKS (12):
     - Retourner inode->block[block_index]
  2. Sinon (bloc indirect):
     - Si indirect_block == 0:
       - Retourner 0 (bloc non alloué)
     - indirect_index = block_index - DIRECT_BLOCKS
     - Lire le bloc indirect_block
     - Récupérer indirect_table[indirect_index]
     - Retourner ce numéro de bloc
  3. Si block_index trop grand:
     - Retourner erreur EFBIG

Complexité: O(1) pour blocs directs, O(1) + 1 I/O pour indirects
```

---

## Allocation de Blocs

### Responsabilités

Gestion de l'espace disque via bitmap d'allocation.

### Opérations

#### 1. Allocation de Bloc

```
Fonction: alloc_block(context)

Entrées:
  - context: Contexte

Sorties:
  - Numéro de bloc alloué
  - 0 si échec

Logique:
  1. Acquérir le verrou
  2. Vérifier free_blocks > 0
     - Si = 0: retourner erreur ENOSPC
  3. Rechercher premier bit libre:
     - Optimisation: parcours par octets
     - for (i = 0; i < bitmap_size_bytes; i++):
       - if (bitmap[i] != 0xFF):
         - Chercher le bit libre dans cet octet
  4. Marquer le bit comme occupé:
     - set_bit(block_bitmap, block_num)
  5. Écrire le bitmap sur disque
  6. Décrémenter free_blocks
  7. Écrire le superbloc
  8. Initialiser le bloc (écrire des zéros):
     - Sécurité: évite fuite de données
  9. Libérer le verrou
  10. Retourner block_num

Complexité: O(n/8) en moyenne, O(n) pire cas
```

#### 2. Libération de Bloc

```
Fonction: free_block(context, block_num)

Entrées:
  - context: Contexte
  - block_num: Numéro de bloc à libérer

Sorties:
  - Succès ou erreur

Logique:
  1. Acquérir le verrou
  2. Valider block_num:
     - >= data_block_start
     - < total_blocks
  3. Vérifier que le bit est occupé:
     - Si déjà libre: journaliser avertissement
  4. Marquer le bit comme libre:
     - clear_bit(block_bitmap, block_num)
  5. Écrire le bitmap
  6. Incrémenter free_blocks
  7. Écrire le superbloc
  8. Libérer le verrou
  9. Retourner succès

Complexité: O(1)
```

#### 3. Allocation Multiple de Blocs

```
Fonction: alloc_blocks(context, count, blocks_out)

Entrées:
  - context: Contexte
  - count: Nombre de blocs à allouer
  - blocks_out: Tableau pour stocker les numéros

Sorties:
  - Nombre de blocs réellement alloués
  - Array de numéros de blocs

Logique:
  1. Acquérir le verrou
  2. Vérifier free_blocks >= count
     - Sinon: ajuster count = free_blocks
  3. Pour i de 0 à count-1:
     - block = find_first_zero_bit(bitmap)
     - Si block trouvé:
       - set_bit(bitmap, block)
       - blocks_out[i] = block
       - allocated++
     - Sinon:
       - break (plus d'espace)
  4. Si allocated > 0:
     - Écrire le bitmap
     - free_blocks -= allocated
     - Écrire le superbloc
  5. Initialiser tous les blocs alloués
  6. Libérer le verrou
  7. Retourner allocated

Optimisation: Recherche de blocs contigus pour réduire fragmentation

Complexité: O(count × n/8)
```

---

## Opérations sur Répertoires

### Responsabilités

Gestion de la hiérarchie de fichiers et répertoires.

### Opérations

#### 1. Listage de Répertoire

```
Fonction: readdir(context, dir_ino, filler_callback)

Entrées:
  - context: Contexte
  - dir_ino: Numéro d'inode du répertoire
  - filler_callback: Fonction pour remplir le buffer

Sorties:
  - Succès ou erreur

Logique:
  1. Lire l'inode du répertoire
  2. Vérifier que c'est un répertoire (mode & S_IFDIR)
  3. Pour chaque bloc de données du répertoire:
     - Lire le bloc
     - offset = 0
     - Tant que offset < block_size:
       - Lire dir_entry à offset
       - Si inode != 0 (entrée valide):
         - Appeler filler_callback(name, stat)
       - offset += rec_len
  4. Retourner succès

Complexité: O(n) où n = nombre d'entrées
```

#### 2. Recherche dans Répertoire

```
Fonction: lookup(context, dir_ino, name, ino_out)

Entrées:
  - context: Contexte
  - dir_ino: Inode du répertoire
  - name: Nom à rechercher
  - ino_out: Pointeur pour stocker l'inode trouvé

Sorties:
  - Numéro d'inode si trouvé
  - 0 si non trouvé

Logique:
  1. Lire l'inode du répertoire
  2. Vérifier que c'est un répertoire
  3. Pour chaque bloc de données:
     - Lire le bloc
     - Pour chaque dir_entry:
       - Si entry->name == name:
         - *ino_out = entry->inode
         - Retourner succès
  4. Retourner ENOENT (non trouvé)

Complexité: O(n) où n = nombre d'entrées
Optimisation possible: Table de hachage
```

#### 3. Ajout d'Entrée dans Répertoire

```
Fonction: add_dir_entry(context, dir_ino, name, ino, file_type)

Entrées:
  - context: Contexte
  - dir_ino: Inode du répertoire parent
  - name: Nom du fichier
  - ino: Inode du fichier
  - file_type: Type (fichier, répertoire, etc.)

Sorties:
  - Succès ou erreur

Logique:
  1. Valider name (longueur <= MAX_FILENAME)
  2. Lire l'inode du répertoire
  3. Vérifier que c'est un répertoire
  4. Vérifier que name n'existe pas déjà:
     - lookup(context, dir_ino, name)
     - Si trouvé: retourner EEXIST
  5. Calculer rec_len nécessaire:
     - Arrondi à multiple de 8 octets
  6. Rechercher espace libre dans les blocs existants:
     - Pour chaque bloc:
       - Vérifier s'il y a assez d'espace
       - Si oui: insérer l'entrée
  7. Si aucun espace trouvé:
     - Allouer un nouveau bloc
     - Ajouter à l'inode du répertoire
     - Insérer l'entrée dans le nouveau bloc
  8. Mettre à jour la taille du répertoire
  9. Mettre à jour mtime du répertoire
  10. Écrire l'inode du répertoire
  11. Retourner succès

Complexité: O(n) + potentiel O(1) allocation
```

#### 4. Suppression d'Entrée de Répertoire

```
Fonction: remove_dir_entry(context, dir_ino, name)

Entrées:
  - context: Contexte
  - dir_ino: Inode du répertoire
  - name: Nom à supprimer

Sorties:
  - Inode du fichier supprimé
  - 0 si non trouvé

Logique:
  1. Lire l'inode du répertoire
  2. Pour chaque bloc de données:
     - Pour chaque dir_entry:
       - Si entry->name == name:
         - Sauvegarder entry->inode
         - Marquer l'entrée comme invalide:
           - entry->inode = 0
           - OU compacter le répertoire
         - Écrire le bloc modifié
         - Mettre à jour mtime du répertoire
         - Retourner inode sauvegardé
  3. Retourner ENOENT si non trouvé

Note: Stratégies de compactage possibles
  - Lazy: Laisser inode = 0 (peut fragmenter)
  - Eager: Déplacer entrées suivantes (coûteux)

Complexité: O(n)
```

---

## Opérations sur Fichiers

### Responsabilités

Lecture et écriture de données dans les fichiers.

### Opérations

#### 1. Lecture de Fichier

```
Fonction: read_file(context, ino, buffer, size, offset)

Entrées:
  - context: Contexte
  - ino: Inode du fichier
  - buffer: Buffer pour stocker les données
  - size: Nombre d'octets à lire
  - offset: Position de départ

Sorties:
  - Nombre d'octets lus
  - Code d'erreur si échec

Logique:
  1. Lire l'inode
  2. Vérifier les permissions de lecture
  3. Ajuster size si offset + size > file_size:
     - size = file_size - offset
  4. Si size <= 0: retourner 0
  5. Calculer les blocs à lire:
     - start_block = offset / block_size
     - end_block = (offset + size - 1) / block_size
  6. Pour chaque bloc de start_block à end_block:
     - Obtenir le numéro de bloc physique:
       - get_block(context, inode, block_index, &block_num)
     - Si block_num == 0:
       - Trou dans le fichier (sparse file)
       - Remplir buffer avec des zéros
     - Sinon:
       - Lire le bloc depuis le disque
     - Calculer start_offset et bytes_to_copy:
       - Premier bloc: offset % block_size
       - Dernier bloc: peut être partiel
     - Copier dans buffer
     - Avancer buffer_position
  7. Mettre à jour atime de l'inode
  8. Écrire l'inode
  9. Retourner bytes_read

Complexité: O(k) où k = nombre de blocs lus
```

#### 2. Écriture de Fichier

```
Fonction: write_file(context, ino, buffer, size, offset)

Entrées:
  - context: Contexte
  - ino: Inode du fichier
  - buffer: Données à écrire
  - size: Nombre d'octets
  - offset: Position d'écriture

Sorties:
  - Nombre d'octets écrits
  - Code d'erreur si échec

Logique:
  1. Lire l'inode
  2. Vérifier les permissions d'écriture
  3. Calculer les blocs nécessaires:
     - start_block = offset / block_size
     - end_block = (offset + size - 1) / block_size
  4. Pour chaque bloc de start_block à end_block:
     - Obtenir block_num = get_block(inode, block_index)
     - Si block_num == 0:
       - Allouer un nouveau bloc:
         - block_num = alloc_block(context)
         - Si échec: retourner ENOSPC
         - Ajouter à l'inode:
           - Si block_index < DIRECT_BLOCKS:
             - inode->block[block_index] = block_num
           - Sinon:
             - Gérer bloc indirect
     - Si écriture partielle (pas tout le bloc):
       - Lire le bloc existant (read-modify-write)
     - Copier les données depuis buffer
     - Écrire le bloc modifié
  5. Mettre à jour file_size si nécessaire:
     - new_size = max(file_size, offset + bytes_written)
     - inode->size = new_size
  6. Mettre à jour mtime et ctime
  7. Incrémenter blocks_count si nouveaux blocs
  8. Écrire l'inode
  9. Retourner bytes_written

Complexité: O(k) où k = nombre de blocs écrits
```

#### 3. Troncature de Fichier

```
Fonction: truncate_file(context, ino, new_size)

Entrées:
  - context: Contexte
  - ino: Inode du fichier
  - new_size: Nouvelle taille

Sorties:
  - Succès ou erreur

Logique:
  1. Lire l'inode
  2. Si new_size == current_size:
     - Retourner succès (rien à faire)
  3. Si new_size > current_size:
     - Extension du fichier (création de trous)
     - Mettre à jour inode->size = new_size
  4. Si new_size < current_size:
     - Réduction du fichier
     - last_block_to_keep = (new_size - 1) / block_size
     - Pour i de last_block_to_keep + 1 à nombre total de blocs:
       - Obtenir block_num
       - Si block_num != 0:
         - free_block(context, block_num)
         - Mettre à 0 dans l'inode
     - Mettre à jour inode->size = new_size
     - Recalculer blocks_count
  5. Mettre à jour mtime et ctime
  6. Écrire l'inode
  7. Retourner succès

Complexité: O(k) où k = nombre de blocs libérés
```

#### 4. Création de Fichier

```
Fonction: create_file(context, parent_ino, name, mode)

Entrées:
  - context: Contexte
  - parent_ino: Inode du répertoire parent
  - name: Nom du fichier
  - mode: Permissions (ex: 0644)

Sorties:
  - Numéro d'inode du nouveau fichier
  - 0 si échec

Logique:
  1. Vérifier que name n'existe pas:
     - lookup(context, parent_ino, name)
     - Si trouvé: retourner EEXIST
  2. Allouer un nouvel inode:
     - ino = alloc_inode(context)
     - Si échec: retourner ENOSPC
  3. Initialiser l'inode:
     - mode = mode | S_IFREG (fichier régulier)
     - uid = current_uid
     - gid = current_gid
     - size = 0
     - atime = mtime = ctime = current_time
     - links_count = 1
     - blocks_count = 0
     - Tous les pointeurs de blocs à 0
  4. Écrire l'inode
  5. Ajouter l'entrée dans le répertoire parent:
     - add_dir_entry(context, parent_ino, name, ino, FT_REG_FILE)
  6. Retourner ino

Complexité: O(1) + O(n) pour add_dir_entry
```

#### 5. Suppression de Fichier

```
Fonction: unlink_file(context, parent_ino, name)

Entrées:
  - context: Contexte
  - parent_ino: Inode du répertoire parent
  - name: Nom du fichier

Sorties:
  - Succès ou erreur

Logique:
  1. Rechercher le fichier:
     - ino = lookup(context, parent_ino, name)
     - Si non trouvé: retourner ENOENT
  2. Lire l'inode
  3. Vérifier que c'est un fichier (pas un répertoire)
  4. Supprimer l'entrée du répertoire:
     - remove_dir_entry(context, parent_ino, name)
  5. Décrémenter links_count:
     - inode->links_count--
  6. Si links_count == 0:
     - Libérer le fichier complètement:
       - free_inode(context, ino)
       - (libère aussi tous les blocs)
  7. Sinon:
     - Écrire l'inode avec links_count mis à jour
  8. Retourner succès

Complexité: O(n) + O(k) si suppression complète
```

---

## Gestion de la Fragmentation

### Responsabilités

Analyse et détection de la fragmentation du système.

### Métrique de Fragmentation

```
Fragmentation d'un fichier = (nombre_de_fragments - 1) / nombre_total_de_blocs

Où un fragment = séquence de blocs contigus
```

### Algorithme de Détection

```
Fonction: analyze_fragmentation(context, ino)

Entrées:
  - context: Contexte
  - ino: Inode à analyser

Sorties:
  - Pourcentage de fragmentation
  - Nombre de fragments

Logique:
  1. Lire l'inode
  2. Si size == 0: retourner 0% (pas fragmenté)
  3. total_blocks = (size + block_size - 1) / block_size
  4. fragments = 0
  5. in_fragment = false
  6. previous_block = 0
  7. Pour i de 0 à total_blocks - 1:
     - Obtenir block_num = get_block(inode, i)
     - Si block_num == 0:
       - Trou dans le fichier (sparse)
       - in_fragment = false
       - continue
     - Si !in_fragment:
       - fragments++
       - in_fragment = true
     - Sinon si block_num != previous_block + 1:
       - Nouveau fragment (non contigu)
       - fragments++
     - previous_block = block_num
  8. Si fragments == 0: retourner 0%
  9. fragmentation = ((fragments - 1) * 100) / total_blocks
  10. Retourner fragmentation

Complexité: O(n) où n = nombre de blocs
```

### Stratégies de Prévention

1. **Allocation First-Fit Améliorée**
   - Rechercher des blocs contigus en priorité
   - Éviter de fragmenter de grands espaces libres

2. **Pré-allocation**
   - Allouer plusieurs blocs d'un coup pour les gros fichiers
   - Réduire les allocations successives

3. **Compaction Lazy**
   - Reporter le compactage jusqu'à nécessité
   - Éviter overhead inutile

---

## Algorithmes Clés

### 1. Find First Zero Bit (Optimisé)

```
Fonction: find_first_zero_bit(bitmap, size)

Entrées:
  - bitmap: Array d'octets
  - size: Nombre total de bits

Sorties:
  - Index du premier bit à 0
  - -1 si tous les bits sont à 1

Logique optimisée:
  1. bytes_count = size / 8
  2. Pour i de 0 à bytes_count - 1:
     - Si bitmap[i] != 0xFF:
       - Pour bit de 0 à 7:
         - Si !(bitmap[i] & (1 << bit)):
           - Retourner i * 8 + bit
  3. Retourner -1

Optimisation: Skip des octets pleins (0xFF)
Complexité: O(n/8) en moyenne, O(n) pire cas
```

### 2. Calcul de Position Physique

```
Fonction: calculate_inode_position(inode_num, sb)

Entrées:
  - inode_num: Numéro d'inode
  - sb: Superbloc

Sorties:
  - Numéro de bloc
  - Offset dans le bloc

Logique:
  1. inode_size = sizeof(minifs_inode_t)  // 256
  2. inodes_per_block = block_size / inode_size  // 16
  3. block_index = inode_num / inodes_per_block
  4. offset_in_block = inode_num % inodes_per_block
  5. physical_block = sb->inode_table_block + block_index
  6. byte_offset = offset_in_block * inode_size
  7. Retourner (physical_block, byte_offset)

Complexité: O(1)
```

### 3. Allocation Contigüe de Blocs

```
Fonction: alloc_contiguous_blocks(context, count)

Entrées:
  - context: Contexte
  - count: Nombre de blocs contigus désirés

Sorties:
  - Premier bloc de la séquence
  - 0 si échec

Logique:
  1. Si count > free_blocks: retourner 0
  2. consecutive = 0
  3. start_block = 0
  4. Pour i de 0 à total_blocks - 1:
     - Si test_bit(bitmap, i) == 0:
       - Si consecutive == 0:
         - start_block = i
       - consecutive++
       - Si consecutive == count:
         - Pour j de start_block à start_block + count - 1:
           - set_bit(bitmap, j)
         - Retourner start_block
     - Sinon:
       - consecutive = 0
  5. Retourner 0 (pas trouvé)

Complexité: O(n)
```

---

## Gestion des Erreurs

### Principes

1. **Validation en entrée** : Toujours valider les paramètres
2. **Codes d'erreur standards** : Utiliser errno (ENOSPC, ENOENT, etc.)
3. **Rollback** : Annuler les modifications partielles en cas d'erreur
4. **Logging** : Journaliser les erreurs critiques

### Exemple de Gestion d'Erreur

```
Fonction: create_file(context, parent_ino, name, mode)

Gestion des erreurs:
  1. Validation des entrées:
     - Si name == NULL: retourner EINVAL
     - Si strlen(name) > MAX_FILENAME: retourner ENAMETOOLONG
     - Si parent_ino invalide: retourner ENOENT
  
  2. Vérification des ressources:
     - Si free_inodes == 0: retourner ENOSPC
  
  3. Allocation avec rollback:
     - ino = alloc_inode(context)
     - Si échec: retourner ENOSPC
     - Initialiser inode
     - Si write_inode échoue:
       - free_inode(context, ino)  // Rollback
       - Retourner EIO
     - Si add_dir_entry échoue:
       - free_inode(context, ino)  // Rollback
       - Retourner code_erreur
  
  4. Logging:
     - Journaliser les opérations critiques
     - Aide au débogage
```

---

## Performance et Optimisation

### Points d'Optimisation

1. **Cache de Blocs**
   - Garder en mémoire les blocs fréquemment accédés
   - LRU (Least Recently Used) pour éviction

2. **Pré-lecture (Read-ahead)**
   - Lire plusieurs blocs à l'avance lors de lectures séquentielles
   - Améliore les performances de streaming

3. **Write-back Cache**
   - Différer les écritures sur disque
   - Grouper plusieurs écritures

4. **Bitmap Cache**
   - Garder les bitmaps en mémoire
   - Éviter lectures/écritures fréquentes

---

## Conclusion

Cette logique métier fournit une base solide pour un système de fichiers fonctionnel. Elle peut être étendue avec :

- Journalisation (journaling)
- Snapshots
- Compression
- Chiffrement
- Quotas
- ACLs (Access Control Lists)

---

**Document maintenu par :** Projet MiniFS  
**Dernière mise à jour :** Mars 2025  
**Version :** 1.0

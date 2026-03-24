# Guide de Dépannage MiniFS

## Table des matières

- [Problèmes d'installation](#problèmes-dinstallation)
- [Problèmes de montage](#problèmes-de-montage)
- [Problèmes de performance](#problèmes-de-performance)
- [Corruption de données](#corruption-de-données)
- [Erreurs système](#erreurs-système)
- [Outils de diagnostic](#outils-de-diagnostic)

---

## Problèmes d'installation

### Erreur : "fuse.h: No such file or directory"

**Cause:** Les en-têtes de développement FUSE ne sont pas installés.

**Solution:**
```bash
# OpenSUSE Tumbleweed
sudo zypper install fuse3-devel

# Vérifier l'installation
pkg-config --cflags fuse3
```

### Erreur de compilation : "undefined reference to `fuse_main_real'"

**Cause:** Le linker ne trouve pas la bibliothèque FUSE.

**Solution:**
```bash
# Vérifier que libfuse3 est installée
ldconfig -p | grep fuse

# Réinstaller si nécessaire
sudo zypper install --force-resolution fuse3

# Recompiler
make clean && make
```

### Erreur : "Permission denied" lors de `make install`

**Cause:** Droits insuffisants pour l'installation système.

**Solution:**
```bash
# Utiliser sudo
sudo make install

# Ou installer dans un répertoire utilisateur
make install PREFIX=$HOME/.local
```

---

## Problèmes de montage

### Erreur : "fuse: device not found, try 'modprobe fuse' first"

**Cause:** Le module noyau FUSE n'est pas chargé.

**Solution:**
```bash
# Charger le module
sudo modprobe fuse

# Vérifier qu'il est chargé
lsmod | grep fuse

# Charger automatiquement au démarrage
echo "fuse" | sudo tee -a /etc/modules-load.d/fuse.conf
```

### Erreur : "Transport endpoint is not connected"

**Cause:** Le système de fichiers est monté mais le processus FUSE a crashé.

**Solution:**
```bash
# Méthode 1 : Forcer le démontage avec fusermount
sudo fusermount3 -u /mnt/minifs

# Méthode 2 : Lazy unmount
sudo umount -l /mnt/minifs

# Méthode 3 : Si tout échoue, vérifier les processus
ps aux | grep minifs
# Tuer le processus si nécessaire
sudo kill -9 <PID>

# Nettoyer et remonter
sudo fusermount3 -u /mnt/minifs
sudo mount.minifs disk.img /mnt/minifs
```

### Erreur : "mountpoint is not empty"

**Cause:** Le répertoire de montage contient déjà des fichiers.

**Solution:**
```bash
# Option 1 : Utiliser un répertoire vide
mkdir -p /tmp/minifs_mount
sudo mount.minifs disk.img /tmp/minifs_mount

# Option 2 : Sauvegarder et vider le répertoire
mv /mnt/minifs /mnt/minifs.backup
mkdir /mnt/minifs
sudo mount.minifs disk.img /mnt/minifs
```

### Erreur : "Invalid superblock magic number"

**Cause:** L'image disque n'est pas formatée ou est corrompue.

**Solution:**
```bash
# Vérifier le fichier
file disk.img

# Si pas formaté, formater
mkfs.minifs disk.img

# Si corrompu, essayer fsck
sudo fsck.minifs disk.img

# Si irréparable, reformater (PERD LES DONNÉES)
mkfs.minifs --force disk.img
```

### Erreur : "Permission denied" lors du montage

**Cause:** Droits insuffisants ou options de sécurité.

**Solution:**
```bash
# Option 1 : Monter en tant que root
sudo mount.minifs disk.img /mnt/minifs

# Option 2 : Ajouter l'option user_allow_other dans /etc/fuse.conf
echo "user_allow_other" | sudo tee -a /etc/fuse.conf

# Option 3 : Utiliser les options FUSE appropriées
minifs_fuse -o allow_other disk.img /mnt/minifs
```

---

## Problèmes de performance

### Lenteur extrême lors des opérations

**Diagnostic:**
```bash
# Mesurer les performances
time dd if=/dev/zero of=/mnt/minifs/testfile bs=1M count=100

# Vérifier la fragmentation
sudo debugfs.minifs disk.img --fragmentation
```

**Solutions possibles:**

**1. Fragmentation excessive (>30%)**
```bash
# Actuellement, défragmentation manuelle nécessaire
# Sauvegarder les données
tar czf backup.tar.gz /mnt/minifs/

# Reformater
sudo umount /mnt/minifs
mkfs.minifs disk.img

# Restaurer
sudo mount.minifs disk.img /mnt/minifs
tar xzf backup.tar.gz -C /mnt/minifs/ --strip-components=2
```

**2. Trop de petits fichiers**
```bash
# Vérifier le nombre d'inodes utilisés
sudo debugfs.minifs disk.img --stats

# Si proche de la limite, reformater avec plus d'inodes
mkfs.minifs disk.img --inodes 8192
```

**3. Opérations I/O intensives**
```bash
# Activer le mode debug pour voir les opérations
minifs_fuse -d -f disk.img /mnt/minifs

# Analyser avec strace
strace -c minifs_fuse disk.img /mnt/minifs
```

### Utilisation mémoire élevée

**Diagnostic:**
```bash
# Vérifier la mémoire utilisée
ps aux | grep minifs_fuse
top -p $(pgrep minifs_fuse)
```

**Solutions:**
```bash
# Option 1 : Vérifier les fuites mémoire (développement)
valgrind --leak-check=full minifs_fuse disk.img /mnt/minifs

# Option 2 : Limiter la mémoire cache
# (fonctionnalité à implémenter dans le code)

# Option 3 : Redémarrer le processus FUSE
sudo umount /mnt/minifs
sudo mount.minifs disk.img /mnt/minifs
```

---

## Corruption de données

### Erreur : "Bitmap inconsistency detected"

**Cause:** Démontage non propre ou crash système.

**Solution:**
```bash
# Démonter si monté
sudo umount /mnt/minifs 2>/dev/null

# Exécuter fsck
sudo fsck.minifs disk.img

# Si réparation automatique échoue
sudo fsck.minifs disk.img --force

# Afficher le rapport détaillé
sudo fsck.minifs disk.img --verbose
```

**Sortie typique de fsck:**
```
fsck.minifs 1.0.0

Checking disk.img...

Pass 1: Checking superblock...
  OK

Pass 2: Checking inode bitmap...
  Found 3 inconsistencies
  Fixing: Inode 145 marked allocated but unused
  Fixing: Inode 267 marked free but in use
  Fixing: Inode 892 marked allocated but unused

Pass 3: Checking block bitmap...
  Found 12 inconsistencies
  Fixing: 12 orphaned blocks freed

Pass 4: Checking directory structure...
  OK

Pass 5: Checking file sizes...
  Found 1 inconsistency
  Fixing: File /data/corrupted.txt size mismatch (expected 4096, was 8192)

Summary:
  16 errors found and fixed
  Filesystem is now clean
```

### Fichiers corrompus ou illisibles

**Diagnostic:**
```bash
# Lister les fichiers avec erreurs
find /mnt/minifs -type f -exec cat {} \; 2>&1 | grep "Input/output error"

# Vérifier un fichier spécifique
sudo debugfs.minifs disk.img --check-file /path/to/file.txt
```

**Solutions:**
```bash
# Essayer de récupérer le fichier
sudo debugfs.minifs disk.img --extract /corrupted/file.txt /tmp/recovered.txt

# Si irrecupérable, supprimer
rm /mnt/minifs/corrupted/file.txt

# Vérifier l'intégrité du système
sudo fsck.minifs disk.img
```

### Perte de données après crash

**Prévention:**
```bash
# 1. Sauvegardes régulières
#!/bin/bash
# backup_minifs.sh
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
sudo umount /mnt/minifs
cp disk.img backups/disk_${TIMESTAMP}.img
sudo mount.minifs disk.img /mnt/minifs

# Automatiser avec cron
crontab -e
# Ajouter : 0 */6 * * * /path/to/backup_minifs.sh
```

**Récupération:**
```bash
# Restaurer depuis la sauvegarde
sudo umount /mnt/minifs
cp backups/disk_20250311_120000.img disk.img
sudo fsck.minifs disk.img
sudo mount.minifs disk.img /mnt/minifs
```

---

## Erreurs système

### Erreur : "Cannot allocate memory"

**Cause:** Système à court de mémoire ou limites dépassées.

**Solution:**
```bash
# Vérifier la mémoire disponible
free -h

# Vérifier les limites
ulimit -a

# Augmenter les limites (temporaire)
ulimit -n 4096  # Nombre de fichiers ouverts
ulimit -v unlimited  # Mémoire virtuelle

# Permanent : éditer /etc/security/limits.conf
echo "* soft nofile 4096" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 8192" | sudo tee -a /etc/security/limits.conf
```

### Erreur : "Too many open files"

**Cause:** Limite de descripteurs de fichiers atteinte.

**Solution:**
```bash
# Vérifier le nombre de fichiers ouverts
lsof | wc -l
lsof | grep minifs | wc -l

# Augmenter la limite système
echo "fs.file-max = 65536" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p

# Augmenter pour le processus
ulimit -n 4096

# Redémarrer MiniFS
sudo umount /mnt/minifs
sudo mount.minifs disk.img /mnt/minifs
```

### Kernel panic ou freeze après montage

**Cause:** Bug dans le code FUSE ou incompatibilité.

**Solution:**
```bash
# Démarrer en mode debug pour voir les logs
minifs_fuse -d -f disk.img /mnt/minifs

# Vérifier les logs système
sudo journalctl -xe | grep -i fuse
sudo dmesg | tail -50

# Vérifier la version de FUSE
fusermount3 --version

# Mettre à jour FUSE si nécessaire
sudo zypper update fuse3
```

---

## Outils de diagnostic

### 1. debugfs.minifs - Explorateur du système de fichiers

```bash
# Afficher les statistiques
sudo debugfs.minifs disk.img --stats

Sortie:
  MiniFS Statistics
  =================
  Volume name: MyVolume
  Block size: 4096 bytes
  Total blocks: 25600 (100.0 MB)
  Free blocks: 23450 (91.6%)
  Total inodes: 4096
  Free inodes: 3892 (95.0%)
  Fragmentation: 12.3%
  Mount count: 15

# Lister tous les inodes
sudo debugfs.minifs disk.img --list-inodes

# Examiner un inode spécifique
sudo debugfs.minifs disk.img --inode 42

# Vérifier la fragmentation
sudo debugfs.minifs disk.img --fragmentation

  Fragmentation Report
  ====================
  Overall fragmentation: 23.4%
  
  Top 10 fragmented files:
  1. /var/log/syslog (45 fragments, 2.5 MB)
  2. /database.db (38 fragments, 15.2 MB)
  3. /video.mp4 (22 fragments, 120 MB)
  ...

# Dump du superbloc
sudo debugfs.minifs disk.img --superblock
```

### 2. Utiliser strace pour déboguer

```bash
# Tracer toutes les opérations système
strace -f minifs_fuse disk.img /mnt/minifs 2>&1 | tee minifs_trace.log

# Tracer uniquement les opérations de fichiers
strace -e trace=file minifs_fuse disk.img /mnt/minifs

# Compter les appels système
strace -c minifs_fuse disk.img /mnt/minifs
```

### 3. Utiliser valgrind pour détecter les fuites mémoire

```bash
# Vérification complète
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind.log \
         minifs_fuse disk.img /mnt/minifs

# Analyser le rapport
less valgrind.log
```

### 4. Vérifier l'intégrité avec fsck

```bash
# Vérification standard
sudo fsck.minifs disk.img

# Vérification en mode verbose
sudo fsck.minifs disk.img --verbose

# Réparation forcée (sans confirmation)
sudo fsck.minifs disk.img --force --yes

# Vérification sans réparation (dry-run)
sudo fsck.minifs disk.img --check-only
```

### 5. Scripts de diagnostic personnalisés

**check_minifs.sh:**
```bash
#!/bin/bash
# Script de diagnostic complet

IMAGE="$1"
MOUNTPOINT="$2"

echo "=== MiniFS Diagnostic Tool ==="
echo "Image: $IMAGE"
echo "Mountpoint: $MOUNTPOINT"
echo ""

# Vérifier que l'image existe
if [ ! -f "$IMAGE" ]; then
    echo "ERROR: Image file not found"
    exit 1
fi

# Vérifier le magic number
echo "1. Checking superblock..."
sudo debugfs.minifs "$IMAGE" --superblock | head -5

# Vérifier l'intégrité
echo ""
echo "2. Running filesystem check..."
sudo fsck.minifs "$IMAGE" --check-only

# Vérifier la fragmentation
echo ""
echo "3. Analyzing fragmentation..."
sudo debugfs.minifs "$IMAGE" --fragmentation | head -10

# Vérifier l'utilisation
echo ""
echo "4. Disk usage..."
sudo debugfs.minifs "$IMAGE" --stats | grep -E "(Free|Total|Fragmentation)"

# Vérifier les processus
if [ -d "$MOUNTPOINT" ]; then
    echo ""
    echo "5. Checking mounted processes..."
    lsof | grep "$MOUNTPOINT" | wc -l
    echo "Open file handles: $?"
fi

echo ""
echo "=== Diagnostic complete ==="
```

**Utilisation:**
```bash
chmod +x check_minifs.sh
./check_minifs.sh disk.img /mnt/minifs
```

---

## Checklist de résolution de problèmes

Suivez cette checklist dans l'ordre :

- [ ] 1. **Vérifier les logs système**
  ```bash
  sudo journalctl -xe | grep -i minifs
  sudo dmesg | tail -100
  ```

- [ ] 2. **Vérifier que FUSE fonctionne**
  ```bash
  lsmod | grep fuse
  fusermount3 --version
  ```

- [ ] 3. **Tester avec une image propre**
  ```bash
  dd if=/dev/zero of=test.img bs=1M count=10
  mkfs.minifs test.img
  mkdir /tmp/test_mount
  sudo mount.minifs test.img /tmp/test_mount
  ```

- [ ] 4. **Vérifier les permissions**
  ```bash
  ls -l disk.img
  ls -ld /mnt/minifs
  id
  ```

- [ ] 5. **Exécuter en mode debug**
  ```bash
  minifs_fuse -d -f disk.img /mnt/minifs
  ```

- [ ] 6. **Vérifier l'intégrité du système de fichiers**
  ```bash
  sudo fsck.minifs disk.img
  ```

- [ ] 7. **Rechercher dans la documentation**
  - README.md
  - FAQ (si disponible)
  - Issues GitHub

- [ ] 8. **Créer un rapport de bug**
  ```bash
  # Collecter les informations
  uname -a > bug_report.txt
  fusermount3 --version >> bug_report.txt
  sudo debugfs.minifs disk.img --stats >> bug_report.txt
  sudo journalctl -xe | grep minifs >> bug_report.txt
  ```

---

## Support et assistance

Si le problème persiste après avoir suivi ce guide :

1. **Vérifier les issues connues** sur le dépôt du projet
2. **Créer une nouvelle issue** avec :
   - Description détaillée du problème
   - Étapes pour reproduire
   - Logs et messages d'erreur
   - Informations système (OS, version FUSE, etc.)
3. **Contacter la communauté** sur les forums appropriés

---

**Dernière mise à jour :** Mars 2025  
**Version du guide :** 1.0.0

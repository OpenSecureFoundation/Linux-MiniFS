# MiniFS - Guide de Démarrage Rapide

## Installation Réussie !

Félicitations, MiniFS est maintenant installé sur votre système.

## Emplacements des Binaires

Les binaires sont installés dans :
- /usr/local/bin/minifs_fuse - Driver FUSE principal
- /usr/local/sbin/mkfs.minifs - Outil de formatage
- /usr/local/sbin/fsck.minifs - Vérificateur de système de fichiers
- /usr/local/sbin/debugfs.minifs - Outil de débogage
- /usr/local/sbin/mount.minifs - Assistant de montage

## Ajout au PATH (si nécessaire)

Si les commandes ne sont pas trouvées :

invite de commande : 
# Ajouter au PATH temporairement
export PATH=$PATH:/usr/local/bin:/usr/local/sbin

# Ajouter au PATH définitivement
echo 'export PATH=$PATH:/usr/local/bin:/usr/local/sbin' >> ~/.invite de commande : rc
source ~/.invite de commande : rc


## Test Rapide

### 1. Créer une Image de Test

invite de commande : 
dd if=/dev/zero of=~/test-minifs.img bs=1M count=100


Cela crée une image de 100 MB.

### 2. Formater avec MiniFS

invite de commande : 
/usr/local/sbin/mkfs.minifs ~/test-minifs.img


Sortie attendue :

Formatting /home/user/test-minifs.img...
Block size: 4096 bytes
Total blocks: 25600 (100.0 MB)
Inode count: 4096

Layout:
  Superblock:      1 block   (0)
  Inode bitmap:    1 blocks (1-1)
  Inode table:     256 blocks (2-257)
  Block bitmap:    1 blocks (258-258)
  Data blocks:     25341 blocks (259-25599)

Available space: 98.9 MB
Root directory created.

Filesystem created successfully.


### 3. Créer le Point de Montage

invite de commande : 
mkdir -p ~/mnt-minifs


### 4. Monter le Système de Fichiers

invite de commande : 
/usr/local/bin/minifs_fuse ~/test-minifs.img ~/mnt-minifs -o default_permissions -f


Options :
- -f : Mode foreground (pour voir les messages)
- -o default_permissions : Utiliser les permissions par défaut

Note : Cette commande reste active. Ouvrez un nouveau terminal pour continuer.

### 5. Utiliser MiniFS (dans un nouveau terminal)

invite de commande : 
# Créer un fichier
echo "Hello MiniFS!" > ~/mnt-minifs/test.txt

# Lire le fichier
cat ~/mnt-minifs/test.txt

# Lister les fichiers
ls -la ~/mnt-minifs/

# Créer plusieurs fichiers
for i in {1..10}; do
  echo "Fichier numéro $i" > ~/mnt-minifs/file_$i.txt
done

# Vérifier
ls ~/mnt-minifs/
cat ~/mnt-minifs/file_5.txt

# Copier un fichier
cp /etc/hosts ~/mnt-minifs/hosts.txt
cat ~/mnt-minifs/hosts.txt


### 6. Démonter (dans le premier terminal)

invite de commande : 
# Appuyer sur Ctrl+C dans le terminal où minifs_fuse est lancé
# OU dans un autre terminal :
fusermount3 -u ~/mnt-minifs
# OU
umount ~/mnt-minifs


### 7. Vérifier l'Intégrité

invite de commande : 
/usr/local/sbin/fsck.minifs ~/test-minifs.img


### 8. Déboguer / Inspecter

invite de commande : 
# Afficher les statistiques
/usr/local/sbin/debugfs.minifs ~/test-minifs.img --stats

# Afficher le superbloc
/usr/local/sbin/debugfs.minifs ~/test-minifs.img --superblock

# Analyser la fragmentation
/usr/local/sbin/debugfs.minifs ~/test-minifs.img --fragmentation

# Afficher un inode spécifique
/usr/local/sbin/debugfs.minifs ~/test-minifs.img --inode 1

# Lister tous les inodes utilisés
/usr/local/sbin/debugfs.minifs ~/test-minifs.img --list-inodes


## Dépannage

### Problème : "commande introuvable"

invite de commande : 
# Utiliser les chemins complets
/usr/local/sbin/mkfs.minifs
/usr/local/bin/minifs_fuse

# OU ajouter au PATH
export PATH=$PATH:/usr/local/bin:/usr/local/sbin


### Problème : "Transport endpoint is not connected"

invite de commande : 
# Démonter proprement
fusermount3 -u ~/mnt-minifs


### Problème : "fuse: device not found"

invite de commande : 
# Charger le module FUSE
sudo modprobe fuse


### Problème : Permission refusée lors du montage

invite de commande : 
# Monter avec sudo
sudo /usr/local/bin/minifs_fuse ~/test-minifs.img ~/mnt-minifs

# OU ajouter votre utilisateur au groupe fuse
sudo usermod -a -G fuse $USER
# Puis se déconnecter/reconnecter


## Exemple Complet de Session

invite de commande : 
# 1. Créer et formater
dd if=/dev/zero of=demo.img bs=1M count=50
/usr/local/sbin/mkfs.minifs demo.img

# 2. Monter (en arrière-plan)
mkdir -p ~/demo-mount
/usr/local/bin/minifs_fuse demo.img ~/demo-mount -o default_permissions &

# 3. Utiliser
echo "Test 1" > ~/demo-mount/file1.txt
echo "Test 2" > ~/demo-mount/file2.txt
ls -l ~/demo-mount/
cat ~/demo-mount/file1.txt

# 4. Statistiques
/usr/local/sbin/debugfs.minifs demo.img --stats

# 5. Démonter
fusermount3 -u ~/demo-mount

# 6. Vérifier
/usr/local/sbin/fsck.minifs demo.img


## Pour Aller Plus Loin

### Utiliser avec un Vrai Périphérique (Attention !)

invite de commande : 
#  ATTENTION : Cela effacera toutes les données !

# 1. Identifier le périphérique (ex: /dev/sdb1)
lsblk

# 2. Démonter si monté
sudo umount /dev/sdb1

# 3. Formater avec MiniFS
sudo /usr/local/sbin/mkfs.minifs /dev/sdb1

# 4. Monter
sudo mkdir -p /mnt/minifs
sudo /usr/local/bin/minifs_fuse /dev/sdb1 /mnt/minifs

# 5. Utiliser normalement
sudo chown $USER:$USER /mnt/minifs
echo "Hello" > /mnt/minifs/test.txt


### Montage Automatique au Démarrage

Créer /etc/fstab entry :

invite de commande : 
/path/to/minifs.img  /mnt/minifs  fuse.minifs_fuse  defaults,user  0  0


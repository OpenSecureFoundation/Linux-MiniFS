#  MiniFS - Guide Complet d'Installation et d'Utilisation

##  Contenu de cette Archive

Cette archive contient TOUT ce dont vous avez besoin :


minifs-COMPLET-GUI.tar.gz
├── minifs-installer.run      Installateur auto-extractible
├── minifs-gui.py              Interface graphique
├── install-gui.sh             Installation de l'interface
├── UNINSTALL_COMPLETE.sh      Désinstallation complète
├── INSTALL_PROPRE.sh          Installation manuelle
├── src/                       Code source complet
├── tools/                     Outils (mkfs, fsck, debugfs)
└── docs/                      Documentation


---

##  INSTALLATION RAPIDE (Recommandée)

### Option 1 : Installateur Automatique (.run)

C'est la méthode la PLUS SIMPLE :

invite de commande :
### 1. Extraire l'archive
tar -xzf minifs-COMPLET-GUI.tar.gz
cd minifs-fuse

### 2. Lancer l'installateur (une seule commande !)
sudo ./minifs-installer.run


C'est TOUT ! L'installateur va :
-  Détecter votre distribution
-  Installer les dépendances (FUSE, GCC, etc.)
-  Compiler MiniFS
-  Installer les binaires
-  Configurer FUSE

---

### Option 2 : Installation Manuelle

Si l'installateur ne fonctionne pas :

invite de commande :
cd minifs-fuse

# 1. Désinstaller les anciennes versions
sudo ./UNINSTALL_COMPLETE.sh

# 2. Installer proprement
sudo ./INSTALL_PROPRE.sh


---

## INSTALLER L'INTERFACE GRAPHIQUE

Après avoir installé MiniFS :

invite de commande :
cd minifs-fuse
sudo ./install-gui.sh


Ensuite, lancez l'interface :

invite de commande :
# Depuis le terminal
minifs-gui

# OU depuis le menu Applications
# Applications > Système > MiniFS Manager


---

##  UTILISATION DE L'INTERFACE GRAPHIQUE

### Onglet 1 : Créer & Formater

1. Nom de l'image : Choisissez où sauvegarder (ex: `/home/user/minifs.img`)
2. Taille (MB) : Taille du disque (ex: 100 MB)
3. Nombre d'inodes : Nombre de fichiers max (ex: 4096)
4. Cliquez "Créer et Formater"

 Votre image MiniFS est créée !

### Onglet 2 : Monter & Utiliser

1. Image : Sélectionnez votre image (ex: `/home/user/minifs.img`)
2. Point de montage : Où monter (ex: `/mnt/minifs`)
3. Cliquez "Monter"

 Votre système de fichiers est maintenant accessible !

4. Utilisez-le comme un dossier normal :
   - Créez des fichiers
   - Copiez des documents
   - Etc.

5. Cliquez "Démonter" quand vous avez fini

### Onglet 3 : Vérifier & Réparer

1. Sélectionnez une image
2. Cliquez "Vérifier le système de fichiers"

 Détecte et répare les erreurs

### Onglet 4 : Statistiques

1. Sélectionnez une image
2. Cliquez "Afficher les statistiques" ou "Analyser la fragmentation"

 Affiche l'état du système de fichiers

---

##  UTILISATION EN LIGNE DE COMMANDE

### 1. Créer une Image

invite de commande :
# Créer une image de 1 GB
dd if=/dev/zero of=bigdisk.img bs=1M count=1024


### 2. Formater avec MiniFS

invite de commande :
/usr/local/sbin/mkfs.minifs bigdisk.img --inodes 8192


Options :
- `--inodes N` : Nombre d'inodes
- `--force` : Forcer (pas de confirmation)

### 3. Monter

invite de commande :
# Créer le point de montage
sudo mkdir -p /mnt/bigdisk

# Monter
/usr/local/bin/minifs_fuse bigdisk.img /mnt/bigdisk -o default_permissions

# OU en arrière-plan
/usr/local/bin/minifs_fuse bigdisk.img /mnt/bigdisk -o default_permissions &


### 4. Utiliser

invite de commande :
# Créer des fichiers
echo "Hello!" > /mnt/bigdisk/hello.txt
cat /mnt/bigdisk/hello.txt

# Copier des fichiers
cp /etc/hosts /mnt/bigdisk/
cp -r ~/Documents /mnt/bigdisk/

# Lister
ls -la /mnt/bigdisk/


### 5. Démonter

invite de commande :
# Méthode 1
fusermount3 -u /mnt/bigdisk

# Méthode 2
sudo umount /mnt/bigdisk


### 6. Vérifier

invite de commande :
/usr/local/sbin/fsck.minifs bigdisk.img


### 7. Statistiques

invite de commande :
# Afficher les stats
/usr/local/sbin/debugfs.minifs bigdisk.img --stats

# Superbloc
/usr/local/sbin/debugfs.minifs bigdisk.img --superblock

# Fragmentation
/usr/local/sbin/debugfs.minifs bigdisk.img --fragmentation

# Lister les inodes
/usr/local/sbin/debugfs.minifs bigdisk.img --list-inodes

# Afficher un inode spécifique
/usr/local/sbin/debugfs.minifs bigdisk.img --inode 1


---

##  Dépannage (problèmes probables)

### Problème : "commande introuvable"

Solution 1 : Utilisez les chemins complets
invite de commande :
/usr/local/sbin/mkfs.minifs


Solution 2 : Ajoutez au PATH
invite de commande :
echo 'export PATH=$PATH:/usr/local/bin:/usr/local/sbin' >> ~/.invite de commande :rc
source ~/.invite de commande :rc


### Problème : Erreur de montage

invite de commande :
# Vérifier que FUSE est chargé
sudo modprobe fuse
lsmod | grep fuse

# Démonter d'abord
fusermount3 -u /mnt/minifs

# Puis remonter
/usr/local/bin/minifs_fuse image.img /mnt/minifs -o default_permissions


### Problème : Permission refusée

invite de commande :
# Monter en tant que root
sudo /usr/local/bin/minifs_fuse image.img /mnt/minifs

# OU ajouter votre utilisateur au groupe fuse
sudo usermod -a -G fuse $USER
# Puis déconnectez-vous/reconnectez-vous


### Problème : L'interface graphique ne se lance pas

invite de commande :
# Installer les dépendances Python GTK
sudo zypper install python3-gobject gtk3

# Lancer en mode debug
minifs-gui 2>&1 | tee gui-debug.log


---

##  Exemples d'Utilisation

### Exemple 1 : Créer un disque de documents

invite de commande :
# 1. Créer et formater
dd if=/dev/zero of=documents.img bs=1M count=500
/usr/local/sbin/mkfs.minifs documents.img

# 2. Monter
mkdir -p ~/Documents-MiniFS
/usr/local/bin/minifs_fuse documents.img ~/Documents-MiniFS -o default_permissions &

# 3. Copier des documents
cp -r ~/Documents/* ~/Documents-MiniFS/

# 4. Vérifier
ls -lh ~/Documents-MiniFS/

# 5. Démonter
fusermount3 -u ~/Documents-MiniFS


### Exemple 2 : Backup de fichiers

invite de commande :
# Créer une image de backup
dd if=/dev/zero of=backup.img bs=1M count=2048
/usr/local/sbin/mkfs.minifs backup.img --inodes 16384

# Monter
sudo mount -o loop backup.img /mnt/backup  # Utilise mount.minifs

# Sauvegarder
cp -r /home/user/important-data /mnt/backup/

# Démonter
sudo umount /mnt/backup


### Exemple 3 : Utiliser avec l'interface graphique (pour ceux qui les aimes bien)

1. Lancez `minifs-gui`
2. Onglet "Créer & Formater"
3. Nom : `~/my-disk.img`
4. Taille : 200 MB
5. Inodes : 4096
6. Cliquez "Créer et Formater"
7. Allez dans l'onglet "Monter & Utiliser"
8. Sélectionnez votre image
9. Cliquez "Monter"
10. Ouvrez votre gestionnaire de fichiers et allez dans `/mnt/minifs`

---

## Support

Si vous rencontrez des problèmes :

1. Consultez ce guide (GUIDE_COMPLET.md)
2. Lisez QUICKSTART.md
3. Vérifiez docs/TROUBLESHOOTING.md
4. Lancez `./UNINSTALL_COMPLETE.sh` puis `./INSTALL_PROPRE.sh`

---

##  Fonctionnalités

-  Création et formatage d'images
-  Montage/démontage FUSE
-  Lecture/écriture de fichiers
-  Création de répertoires
-  Vérification et réparation (fsck)
-  Statistiques détaillées
-  Analyse de fragmentation
-  Interface graphique GTK
-  Installateur automatique

---

##  Félicitations !

Vous disposez maintenant d'un système de fichiers MiniFS complet et fonctionnel avec :
-  Tous les outils en ligne de commande
-  Une interface graphique moderne
-  Un installateur automatique
-  Une documentation complète

Profitez de MiniFS ! 

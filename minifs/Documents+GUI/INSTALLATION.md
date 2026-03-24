# Guide d'Installation MiniFS

## IMPORTANT - Prérequis

Avant de compiler MiniFS, vous DEVEZ installer FUSE3 sur votre système OpenSUSE.

### Installation des Dépendances sur OpenSUSE Tumbleweed

```invite de commande :
# 1. Mettre à jour le système
sudo zypper refresh
sudo zypper update

# 2. Installer TOUS les paquets nécessaires
sudo zypper install -y \
    gcc \
    make \
    fuse3 \
    fuse3-devel \
    pkg-config \
    git

# 3. Vérifier l'installation de FUSE
pkg-config --modversion fuse3
# Devrait afficher: 3.x.x

# 4. Vérifier que les headers sont présents
ls /usr/include/fuse3/fuse.h
# Devrait exister
```

### Si FUSE3 n'est pas disponible via zypper

```invite de commande :
# Alternative: Compiler FUSE3 depuis les sources
cd /tmp
wget https://github.com/libfuse/libfuse/releases/download/fuse-3.10.5/fuse-3.10.5.tar.xz
tar -xf fuse-3.10.5.tar.xz
cd fuse-3.10.5
mkdir build
cd build
meson ..
ninja
sudo ninja install
sudo ldconfig
```

##  Compilation de MiniFS

Une fois FUSE3 installé :

```invite de commande :
# 1. Extraire l'archive
tar -xzf minifs-FIXED.tar.gz
cd minifs-fuse

# 2. Vérifier que FUSE est détecté
pkg-config --cflags fuse3
# Devrait afficher: -I/usr/include/fuse3 -D_FILE_OFFSET_BITS=64

# 3. Compiler
make clean
make

# 4. Si la compilation réussit, installer
sudo make install
```

##  Vérification de l'Installation

```invite de commande :
# Vérifier que les binaires sont installés
which mkfs.minifs
which minifs_fuse
which fsck.minifs

# Tester mkfs
mkfs.minifs --version
# Devrait afficher: mkfs.minifs version 1.0.0
```

##  Premier Test

```invite de commande :
# 1. Créer une image de test
dd if=/dev/zero of=test.img bs=1M count=50

# 2. Formater
mkfs.minifs test.img

# 3. Créer le point de montage
mkdir -p /mnt/minifs

# 4. Monter
sudo minifs_fuse test.img /mnt/minifs -o default_permissions

# 5. Tester
echo "Hello MiniFS!" > /mnt/minifs/test.txt
cat /mnt/minifs/test.txt
ls -la /mnt/minifs/

# 6. Démonter
sudo umount /mnt/minifs
```

## Résolution de Problèmes probables

### Erreur: "fuse3/fuse.h: No such file or directory"

Cause: FUSE3-devel n'est pas installé

Solution:
```invite de commande :
sudo zypper install fuse3-devel
```

### Erreur: "pthread_mutex_t: unknown type"

Cause: Header pthread manquant (ne devrait plus arriver)

Solution: Le code est déjà corrigé, recompilez

### Erreur de montage: "fuse: device not found"

Cause: Module FUSE pas chargé

Solution:
```invite de commande :
sudo modprobe fuse
lsmod | grep fuse
```

### Erreur: "Permission denied" lors du montage

Solution:
```invite de commande :
# Option 1: Monter en tant que root
sudo minifs_fuse test.img /mnt/minifs

# Option 2: Ajouter votre utilisateur au groupe fuse
sudo usermod -a -G fuse $USER
# Puis déconnectez-vous et reconnectez-vous
```

## Après l'Installation

Consultez la documentation complète :
- `README.md` - Guide complet d'utilisation
- `docs/TROUBLESHOOTING.md` - Guide de dépannage
- `docs/GUIDE_DEVELOPPEMENT.md` - Pour le développement

## IDE Recommandé

Visual Studio Code avec les extensions C/C++

Installation:
```invite de commande :
# Ajouter le dépôt Microsoft
sudo rpm --import https://packages.microsoft.com/keys/microsoft.asc
sudo zypper addrepo https://packages.microsoft.com/yumrepos/vscode vscode
sudo zypper refresh
sudo zypper install code

# Installer les extensions
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.makefile-tools
```

Puis ouvrir le projet:
```invite de commande :
code /path/to/minifs-fuse
```

---
PS: En cas de problème persistant, vérifiez d'abord que FUSE3 est correctement installé avec `pkg-config --modversion fuse3`

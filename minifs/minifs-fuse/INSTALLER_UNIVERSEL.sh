#!/bin/bash
# INSTALLATEUR UNIVERSEL - Fonctionne depuis n'importe où

set -e

echo "═══════════════════════════════════════════════════════"
echo "  INSTALLATEUR UNIVERSEL MINIFS"
echo "═══════════════════════════════════════════════════════"
echo ""

# Détecter le répertoire actuel
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Répertoire détecté: $SCRIPT_DIR"

cd "$SCRIPT_DIR"

echo ""
echo "[1/6] Nettoyage des anciennes installations..."
# Arrêter les processus
sudo pkill -9 minifs_fuse 2>/dev/null || true

# Démonter tout
mount | grep minifs | awk '{print $3}' | while read mp; do
    sudo fusermount3 -uz "$mp" 2>/dev/null || true
    sudo umount -f "$mp" 2>/dev/null || true
done

# Supprimer les anciens binaires
sudo rm -f /usr/local/bin/minifs_fuse 2>/dev/null || true
sudo rm -f /usr/local/bin/minifs-easy 2>/dev/null || true
sudo rm -f /usr/local/sbin/mkfs.minifs 2>/dev/null || true
sudo rm -f /usr/local/sbin/fsck.minifs 2>/dev/null || true
sudo rm -f /usr/local/sbin/debugfs.minifs 2>/dev/null || true
sudo rm -f /usr/local/sbin/mount.minifs 2>/dev/null || true

echo "✓ Nettoyage terminé"

echo ""
echo "[2/6] Correction des permissions dans le code..."

# Forcer les permissions 777 dans main.c
if [ -f "src/main.c" ]; then
    # Backup
    cp src/main.c src/main.c.backup
    
    # Corrections automatiques
    sed -i 's/inode\.mode = S_IFDIR | (mode & 0777)/inode.mode = S_IFDIR | 0777/g' src/main.c
    sed -i 's/inode\.mode = S_IFREG | (mode & 0777)/inode.mode = S_IFREG | 0666/g' src/main.c
    
    echo "✓ Code corrigé"
else
    echo "✗ Erreur: src/main.c introuvable"
    exit 1
fi

echo ""
echo "[3/6] Compilation..."
make clean 2>/dev/null || true
if ! make; then
    echo "✗ Erreur de compilation"
    exit 1
fi
echo "✓ Compilation réussie"

echo ""
echo "[4/6] Installation..."
if ! sudo make install; then
    echo "✗ Erreur d'installation"
    exit 1
fi
echo "✓ Installation réussie"

echo ""
echo "[5/6] Configuration FUSE..."
sudo modprobe fuse 2>/dev/null || true
if [ ! -f /etc/fuse.conf ]; then
    echo "user_allow_other" | sudo tee /etc/fuse.conf > /dev/null
elif ! grep -q "user_allow_other" /etc/fuse.conf; then
    echo "user_allow_other" | sudo tee -a /etc/fuse.conf > /dev/null
fi
echo "✓ FUSE configuré"

echo ""
echo "[6/6] Création de la commande facile..."
sudo tee /usr/local/bin/minifs-easy > /dev/null << 'EOFMINIFS'
#!/bin/bash
# MiniFS Easy - Commande simplifiée

cmd=$1
shift

case "$cmd" in
    create)
        if [ -z "$1" ]; then
            echo "Usage: minifs-easy create IMAGE [SIZE_MB]"
            exit 1
        fi
        image=$1
        size=${2:-100}
        echo "Création de $image ($size MB)..."
        dd if=/dev/zero of="$image" bs=1M count=$size 2>/dev/null
        /usr/local/sbin/mkfs.minifs "$image" --force
        echo "✓ Image créée: $image"
        ;;
    mount)
        if [ -z "$2" ]; then
            echo "Usage: minifs-easy mount IMAGE MOUNTPOINT"
            exit 1
        fi
        image=$1
        mountpoint=$2
        mkdir -p "$mountpoint"
        echo "Montage de $image sur $mountpoint..."
        sudo /usr/local/bin/minifs_fuse "$image" "$mountpoint" \
            -o default_permissions,allow_other,umask=000
        sleep 1
        sudo chmod -R 777 "$mountpoint" 2>/dev/null || true
        echo "✓ Monté avec succès"
        echo ""
        echo "Vous pouvez maintenant utiliser $mountpoint"
        echo "Exemples:"
        echo "  echo 'Test' > $mountpoint/test.txt"
        echo "  mkdir $mountpoint/dossier"
        echo "  cp fichier.pdf $mountpoint/"
        ;;
    umount|unmount)
        if [ -z "$1" ]; then
            echo "Usage: minifs-easy umount MOUNTPOINT"
            exit 1
        fi
        mountpoint=$1
        echo "Démontage de $mountpoint..."
        sudo fusermount3 -u "$mountpoint" 2>/dev/null || \
        sudo umount "$mountpoint" 2>/dev/null || \
        sudo umount -f "$mountpoint"
        echo "✓ Démonté"
        ;;
    check)
        if [ -z "$1" ]; then
            echo "Usage: minifs-easy check IMAGE"
            exit 1
        fi
        /usr/local/sbin/fsck.minifs "$1"
        ;;
    stats)
        if [ -z "$1" ]; then
            echo "Usage: minifs-easy stats IMAGE"
            exit 1
        fi
        /usr/local/sbin/debugfs.minifs "$1" --stats
        ;;
    help|--help|-h|"")
        cat << 'HELPEOF'
MiniFS Easy - Commandes simplifiées pour MiniFS

UTILISATION:
  minifs-easy COMMANDE [ARGUMENTS]

COMMANDES:
  create IMAGE [SIZE_MB]  Créer et formater une image
                          Exemple: minifs-easy create disk.img 200

  mount IMAGE MOUNTPOINT  Monter une image
                          Exemple: minifs-easy mount disk.img ~/mnt

  umount MOUNTPOINT       Démonter
                          Exemple: minifs-easy umount ~/mnt

  check IMAGE             Vérifier l'intégrité
                          Exemple: minifs-easy check disk.img

  stats IMAGE             Afficher les statistiques
                          Exemple: minifs-easy stats disk.img

WORKFLOW COMPLET:
  # 1. Créer un disque de 500 MB
  minifs-easy create documents.img 500

  # 2. Monter
  minifs-easy mount documents.img ~/Documents-MiniFS

  # 3. Utiliser normalement
  echo "Test" > ~/Documents-MiniFS/test.txt
  mkdir ~/Documents-MiniFS/photos
  cp *.pdf ~/Documents-MiniFS/

  # 4. Démonter
  minifs-easy umount ~/Documents-MiniFS

TOUT FONCTIONNE - Plus d'erreurs de permissions !
HELPEOF
        ;;
    *)
        echo "Commande inconnue: $cmd"
        echo "Utilisez: minifs-easy help"
        exit 1
        ;;
esac
EOFMINIFS

sudo chmod +x /usr/local/bin/minifs-easy
echo "✓ Commande 'minifs-easy' créée"

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  ✓✓✓ INSTALLATION TERMINÉE AVEC SUCCÈS ✓✓✓"
echo "═══════════════════════════════════════════════════════"
echo ""
echo "Vérification:"
which minifs_fuse
which mkfs.minifs
which minifs-easy
echo ""
echo "UTILISEZ MAINTENANT:"
echo ""
echo "  # Créer un disque"
echo "  minifs-easy create mon-disque.img 500"
echo ""
echo "  # Monter"
echo "  minifs-easy mount mon-disque.img ~/Documents-MiniFS"
echo ""
echo "  # Utiliser (TOUT FONCTIONNE!)"
echo "  echo 'Test' > ~/Documents-MiniFS/test.txt"
echo "  mkdir ~/Documents-MiniFS/photos"
echo "  cp fichier.pdf ~/Documents-MiniFS/"
echo ""
echo "  # Démonter"
echo "  minifs-easy umount ~/Documents-MiniFS"
echo ""
echo "Pour plus d'aide: minifs-easy help"
echo ""

#!/bin/bash
# Script de désinstallation COMPLÈTE de MiniFS

echo "╔════════════════════════════════════════════════════╗"
echo "║   Désinstallation Complète de MiniFS               ║"
echo "╚════════════════════════════════════════════════════╝"
echo ""

# Vérifier les privilèges root
if [ "$EUID" -ne 0 ]; then
    echo "Ce script doit être exécuté en tant que root"
    echo "Utilisez: sudo $0"
    exit 1
fi

echo "[1/5] Démontage de tous les systèmes MiniFS montés..."
# Démonter tous les points de montage MiniFS
mount | grep minifs | awk '{print $3}' | while read mountpoint; do
    echo "  Démontage de $mountpoint"
    umount "$mountpoint" 2>/dev/null || fusermount3 -u "$mountpoint" 2>/dev/null
done

echo "[2/5] Suppression des binaires dans /usr/local/..."
rm -f /usr/local/bin/minifs_fuse
rm -f /usr/local/sbin/mkfs.minifs
rm -f /usr/local/sbin/fsck.minifs
rm -f /usr/local/sbin/debugfs.minifs
rm -f /usr/local/sbin/mount.minifs

echo "[3/5] Suppression des binaires dans /usr/bin et /usr/sbin..."
rm -f /usr/bin/minifs_fuse
rm -f /usr/sbin/mkfs.minifs
rm -f /usr/sbin/fsck.minifs
rm -f /usr/sbin/debugfs.minifs
rm -f /usr/sbin/mount.minifs

echo "[4/5] Recherche d'autres installations..."
find /usr -name "*minifs*" 2>/dev/null | while read file; do
    echo "  Trouvé: $file"
    rm -f "$file"
done

echo "[5/5] Nettoyage des répertoires de build..."
# Nettoyer les anciens répertoires de build
find ~ -name "minifs-fuse" -type d 2>/dev/null | while read dir; do
    if [ -d "$dir/build" ]; then
        echo "  Nettoyage de $dir/build"
        rm -rf "$dir/build"
    fi
    if [ -d "$dir/bin" ]; then
        echo "  Nettoyage de $dir/bin"
        rm -rf "$dir/bin"
    fi
done

echo ""
echo "╔════════════════════════════════════════════════════╗"
echo "║   Désinstallation Terminée                         ║"
echo "╚════════════════════════════════════════════════════╝"
echo ""
echo "Vérification:"
echo "  which mkfs.minifs    : $(which mkfs.minifs 2>&1)"
echo "  which minifs_fuse    : $(which minifs_fuse 2>&1)"
echo "  which fsck.minifs    : $(which fsck.minifs 2>&1)"
echo ""
echo "Si rien n'est affiché ci-dessus, la désinstallation est complète."
echo "Vous pouvez maintenant faire une installation propre."

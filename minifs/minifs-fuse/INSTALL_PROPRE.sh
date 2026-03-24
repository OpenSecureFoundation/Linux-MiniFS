#!/bin/bash
# Installation PROPRE de MiniFS

echo "╔════════════════════════════════════════════════════╗"
echo "║   Installation Propre de MiniFS                    ║"
echo "║   Version 1.0.0                                    ║"
echo "╚════════════════════════════════════════════════════╝"
echo ""

# Vérifier les privilèges root
if [ "$EUID" -ne 0 ]; then
    echo "❌ Ce script doit être exécuté en tant que root"
    echo "   Utilisez: sudo $0"
    exit 1
fi

# Vérifier FUSE
echo "[1/6] Vérification de FUSE..."
if pkg-config --exists fuse3 2>/dev/null; then
    echo "✓ FUSE3 détecté"
elif pkg-config --exists fuse 2>/dev/null; then
    echo "✓ FUSE détecté"
else
    echo "❌ FUSE non détecté"
    echo "   Installation de FUSE..."
    zypper --non-interactive install fuse3 fuse3-devel || exit 1
fi

# Nettoyer les anciennes compilations
echo "[2/6] Nettoyage des anciennes compilations..."
make clean 2>/dev/null

# Compiler
echo "[3/6] Compilation de MiniFS..."
if make; then
    echo "✓ Compilation réussie"
else
    echo "❌ Échec de la compilation"
    exit 1
fi

# Installer
echo "[4/6] Installation des binaires..."
if make install; then
    echo "✓ Installation réussie"
else
    echo "❌ Échec de l'installation"
    exit 1
fi

# Configurer FUSE
echo "[5/6] Configuration de FUSE..."
modprobe fuse 2>/dev/null
if [ ! -f /etc/modules-load.d/fuse.conf ]; then
    echo "fuse" > /etc/modules-load.d/fuse.conf
fi
if [ ! -f /etc/fuse.conf ]; then
    touch /etc/fuse.conf
fi
if ! grep -q "user_allow_other" /etc/fuse.conf 2>/dev/null; then
    echo "user_allow_other" >> /etc/fuse.conf
fi

# Test
echo "[6/6] Test de l'installation..."
TEST_IMG="/tmp/minifs_test_$$.img"
dd if=/dev/zero of="$TEST_IMG" bs=1M count=5 &>/dev/null

if /usr/local/sbin/mkfs.minifs "$TEST_IMG" --force &>/dev/null; then
    echo "✓ Test de formatage OK"
    rm -f "$TEST_IMG"
else
    echo "❌ Test de formatage échoué"
    rm -f "$TEST_IMG"
    exit 1
fi

echo ""
echo "╔════════════════════════════════════════════════════╗"
echo "║   Installation Terminée avec Succès!               ║"
echo "╚════════════════════════════════════════════════════╝"
echo ""
echo "Binaires installés:"
echo "  /usr/local/bin/minifs_fuse"
echo "  /usr/local/sbin/mkfs.minifs"
echo "  /usr/local/sbin/fsck.minifs"
echo "  /usr/local/sbin/debugfs.minifs"
echo "  /usr/local/sbin/mount.minifs"
echo ""
echo "Test rapide:"
echo "  dd if=/dev/zero of=test.img bs=1M count=50"
echo "  /usr/local/sbin/mkfs.minifs test.img"
echo "  mkdir -p ~/mnt"
echo "  /usr/local/bin/minifs_fuse test.img ~/mnt -o default_permissions &"
echo "  echo 'Hello!' > ~/mnt/test.txt"
echo "  cat ~/mnt/test.txt"
echo "  fusermount3 -u ~/mnt"

#!/bin/bash
# Script pour créer un installateur auto-extractible

echo "Création de l'installateur MiniFS..."

# Créer le script d'installation
cat > /tmp/installer_payload.sh << 'PAYLOAD'
#!/bin/bash
# MiniFS Auto-Installer

INSTALL_DIR="/tmp/minifs-install-$$"
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "╔════════════════════════════════════════════════════════╗"
echo "║                                                        ║"
echo "║          MiniFS - Installation Automatique             ║"
echo "║                  Version 1.0.0                         ║"
echo "║                                                        ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Vérifier root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Ce script doit être exécuté en tant que root${NC}"
    echo "Utilisez: sudo $0"
    exit 1
fi

# Extraire l'archive
echo "[1/5] Extraction de l'archive..."
ARCHIVE_LINE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $0)
mkdir -p "$INSTALL_DIR"
tail -n+$ARCHIVE_LINE $0 | tar xz -C "$INSTALL_DIR"
cd "$INSTALL_DIR/minifs-fuse" || exit 1

# Installer les dépendances
echo "[2/5] Installation des dépendances..."
if command -v zypper &> /dev/null; then
    zypper --non-interactive install fuse3 fuse3-devel gcc make pkg-config
elif command -v apt-get &> /dev/null; then
    apt-get update
    apt-get install -y fuse3 libfuse3-dev gcc make pkg-config
elif command -v dnf &> /dev/null; then
    dnf install -y fuse3 fuse3-devel gcc make pkgconfig
else
    echo -e "${RED}Gestionnaire de paquets non supporté${NC}"
    exit 1
fi

# Compiler
echo "[3/5] Compilation..."
make clean
if ! make; then
    echo -e "${RED}Échec de la compilation${NC}"
    exit 1
fi

# Installer
echo "[4/5] Installation..."
if ! make install; then
    echo -e "${RED}Échec de l'installation${NC}"
    exit 1
fi

# Configuration
echo "[5/5] Configuration finale..."
modprobe fuse 2>/dev/null
[ ! -f /etc/modules-load.d/fuse.conf ] && echo "fuse" > /etc/modules-load.d/fuse.conf
[ ! -f /etc/fuse.conf ] && touch /etc/fuse.conf
grep -q "user_allow_other" /etc/fuse.conf 2>/dev/null || echo "user_allow_other" >> /etc/fuse.conf

# Nettoyage
cd /
rm -rf "$INSTALL_DIR"

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                                                        ║${NC}"
echo -e "${GREEN}║       Installation Terminée avec Succès!               ║${NC}"
echo -e "${GREEN}║                                                        ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Binaires installés:"
echo "  /usr/local/bin/minifs_fuse"
echo "  /usr/local/sbin/mkfs.minifs"
echo "  /usr/local/sbin/fsck.minifs"
echo "  /usr/local/sbin/debugfs.minifs"
echo ""
echo "Exemple d'utilisation:"
echo "  dd if=/dev/zero of=test.img bs=1M count=100"
echo "  /usr/local/sbin/mkfs.minifs test.img"
echo "  mkdir -p /mnt/minifs"
echo "  /usr/local/bin/minifs_fuse test.img /mnt/minifs -o default_permissions"
echo ""

exit 0

__ARCHIVE_BELOW__
PAYLOAD

# Créer l'archive
cd /home/claude
tar czf /tmp/minifs-archive.tar.gz minifs-fuse/

# Combiner le script et l'archive
cat /tmp/installer_payload.sh /tmp/minifs-archive.tar.gz > /home/claude/minifs-fuse/minifs-installer.run
chmod +x /home/claude/minifs-fuse/minifs-installer.run

# Nettoyer
rm /tmp/installer_payload.sh /tmp/minifs-archive.tar.gz

echo "✓ Installateur créé: minifs-installer.run"
ls -lh /home/claude/minifs-fuse/minifs-installer.run

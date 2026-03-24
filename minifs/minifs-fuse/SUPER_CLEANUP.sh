#!/bin/bash
# SUPER NETTOYAGE COMPLET - Supprime TOUT ce qui concerne MiniFS

set +e  # Continuer même en cas d'erreur

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${RED}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${RED}║                                                        ║${NC}"
echo -e "${RED}║     SUPER NETTOYAGE COMPLET DE MINIFS                  ║${NC}"
echo -e "${RED}║     Suppression de TOUT                                ║${NC}"
echo -e "${RED}║                                                        ║${NC}"
echo -e "${RED}╚════════════════════════════════════════════════════════╝${NC}"
echo ""

# Vérifier root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Ce script DOIT être exécuté en tant que root${NC}"
    echo "Utilisez: sudo $0"
    exit 1
fi

echo -e "${YELLOW}⚠️  ATTENTION: Ce script va supprimer TOUT ce qui concerne MiniFS${NC}"
echo "   - Tous les dossiers minifs-fuse dans TOUS les répertoires"
echo "   - Tous les binaires installés"
echo "   - Tous les points de montage"
echo "   - Tous les processus"
echo ""
read -p "Voulez-vous vraiment continuer? (tapez OUI en majuscules): " confirm

if [ "$confirm" != "OUI" ]; then
    echo "Annulé."
    exit 0
fi

echo ""
echo -e "${BLUE}[ÉTAPE 1/10]${NC} Démontage de tous les systèmes MiniFS..."
mount | grep -i minifs | awk '{print $3}' | while read mp; do
    echo "  Démontage de $mp"
    fusermount3 -uz "$mp" 2>/dev/null
    umount -f "$mp" 2>/dev/null
    umount -l "$mp" 2>/dev/null
done

mount | grep -i fuse | grep -v "fusectl" | awk '{print $3}' | while read mp; do
    if [ -d "$mp" ] && [ "$(ls -A $mp 2>/dev/null)" ]; then
        echo "  Vérification de $mp"
        fusermount3 -uz "$mp" 2>/dev/null
    fi
done

echo -e "${GREEN}✓${NC} Démontage terminé"

echo ""
echo -e "${BLUE}[ÉTAPE 2/10]${NC} Arrêt de tous les processus minifs_fuse..."
pkill -9 minifs_fuse 2>/dev/null
pkill -9 mkfs.minifs 2>/dev/null
pkill -9 fsck.minifs 2>/dev/null
pkill -9 debugfs.minifs 2>/dev/null
echo -e "${GREEN}✓${NC} Processus arrêtés"

echo ""
echo -e "${BLUE}[ÉTAPE 3/10]${NC} Suppression des binaires dans /usr/local/..."
rm -fv /usr/local/bin/minifs_fuse 2>/dev/null
rm -fv /usr/local/bin/minifs-gui 2>/dev/null
rm -fv /usr/local/sbin/mkfs.minifs 2>/dev/null
rm -fv /usr/local/sbin/fsck.minifs 2>/dev/null
rm -fv /usr/local/sbin/debugfs.minifs 2>/dev/null
rm -fv /usr/local/sbin/mount.minifs 2>/dev/null
echo -e "${GREEN}✓${NC} Binaires /usr/local supprimés"

echo ""
echo -e "${BLUE}[ÉTAPE 4/10]${NC} Suppression des binaires dans /usr/..."
rm -fv /usr/bin/minifs_fuse 2>/dev/null
rm -fv /usr/bin/minifs-gui 2>/dev/null
rm -fv /usr/sbin/mkfs.minifs 2>/dev/null
rm -fv /usr/sbin/fsck.minifs 2>/dev/null
rm -fv /usr/sbin/debugfs.minifs 2>/dev/null
rm -fv /usr/sbin/mount.minifs 2>/dev/null
echo -e "${GREEN}✓${NC} Binaires /usr supprimés"

echo ""
echo -e "${BLUE}[ÉTAPE 5/10]${NC} Recherche de tous les dossiers minifs-fuse..."
echo "  Recherche dans /home..."
find /home -type d -name "*minifs*" 2>/dev/null | while read dir; do
    echo "    Trouvé: $dir"
    echo "    Suppression..."
    rm -rf "$dir" 2>/dev/null
done

echo "  Recherche dans /root..."
find /root -type d -name "*minifs*" 2>/dev/null | while read dir; do
    echo "    Trouvé: $dir"
    echo "    Suppression..."
    rm -rf "$dir" 2>/dev/null
done

echo "  Recherche dans /tmp..."
find /tmp -type d -name "*minifs*" 2>/dev/null | while read dir; do
    echo "    Trouvé: $dir"
    echo "    Suppression..."
    rm -rf "$dir" 2>/dev/null
done

echo "  Recherche dans /var..."
find /var -type d -name "*minifs*" 2>/dev/null | while read dir; do
    echo "    Trouvé: $dir"
    echo "    Suppression..."
    rm -rf "$dir" 2>/dev/null
done

echo -e "${GREEN}✓${NC} Dossiers supprimés"

echo ""
echo -e "${BLUE}[ÉTAPE 6/10]${NC} Suppression des fichiers .img (images MiniFS)..."
find /home -type f -name "*.img" 2>/dev/null | while read img; do
    # Vérifier si c'est bien une image MiniFS
    if file "$img" | grep -q "data"; then
        size=$(du -h "$img" | cut -f1)
        echo "  Trouvé: $img ($size)"
        read -p "    Supprimer? (o/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Oo]$ ]]; then
            rm -f "$img"
            echo "    Supprimé"
        fi
    fi
done
echo -e "${GREEN}✓${NC} Images vérifiées"

echo ""
echo -e "${BLUE}[ÉTAPE 7/10]${NC} Nettoyage des points de montage..."
# Supprimer les répertoires de montage vides
for dir in /mnt/minifs /mnt/bigdisk /mnt/minifs-test; do
    if [ -d "$dir" ]; then
        if [ -z "$(ls -A $dir)" ]; then
            echo "  Suppression de $dir (vide)"
            rmdir "$dir" 2>/dev/null
        else
            echo "  $dir n'est pas vide, conservation"
        fi
    fi
done
echo -e "${GREEN}✓${NC} Points de montage nettoyés"

echo ""
echo -e "${BLUE}[ÉTAPE 8/10]${NC} Suppression du fichier .desktop..."
rm -fv /usr/share/applications/minifs.desktop 2>/dev/null
echo -e "${GREEN}✓${NC} Fichier .desktop supprimé"

echo ""
echo -e "${BLUE}[ÉTAPE 9/10]${NC} Recherche d'autres fichiers MiniFS..."
find /usr -name "*minifs*" 2>/dev/null | while read file; do
    echo "  Trouvé: $file"
    rm -f "$file" 2>/dev/null
done
echo -e "${GREEN}✓${NC} Autres fichiers nettoyés"

echo ""
echo -e "${BLUE}[ÉTAPE 10/10]${NC} Vérification finale..."
echo ""
echo "Vérification des binaires:"
echo -n "  mkfs.minifs:    "
if command -v mkfs.minifs &> /dev/null; then
    echo -e "${RED}ENCORE PRÉSENT: $(which mkfs.minifs)${NC}"
else
    echo -e "${GREEN}✓ Supprimé${NC}"
fi

echo -n "  minifs_fuse:    "
if command -v minifs_fuse &> /dev/null; then
    echo -e "${RED}ENCORE PRÉSENT: $(which minifs_fuse)${NC}"
else
    echo -e "${GREEN}✓ Supprimé${NC}"
fi

echo -n "  fsck.minifs:    "
if command -v fsck.minifs &> /dev/null; then
    echo -e "${RED}ENCORE PRÉSENT: $(which fsck.minifs)${NC}"
else
    echo -e "${GREEN}✓ Supprimé${NC}"
fi

echo ""
echo "Vérification des dossiers:"
remaining=$(find /home /root /tmp -type d -name "*minifs*" 2>/dev/null | wc -l)
if [ "$remaining" -eq 0 ]; then
    echo -e "  ${GREEN}✓ Aucun dossier minifs restant${NC}"
else
    echo -e "  ${YELLOW}⚠ $remaining dossiers minifs encore présents:${NC}"
    find /home /root /tmp -type d -name "*minifs*" 2>/dev/null | head -10
fi

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                                                        ║${NC}"
echo -e "${GREEN}║     NETTOYAGE TERMINÉ                                  ║${NC}"
echo -e "${GREEN}║                                                        ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Votre système est maintenant PROPRE."
echo "Vous pouvez maintenant installer MiniFS proprement."
echo ""
echo "Commandes recommandées:"
echo "  1. Extraire minifs-FINAL.tar.gz dans UN SEUL endroit"
echo "  2. cd minifs-fuse"
echo "  3. sudo ./scripts/install.sh"
echo ""

#!/bin/bash
# Installation de l'interface graphique MiniFS

echo "Installation de MiniFS GUI..."

# Vérifier root
if [ "$EUID" -ne 0 ]; then
    echo "Ce script doit être exécuté en tant que root"
    echo "Utilisez: sudo $0"
    exit 1
fi

# Installer les dépendances Python/GTK
echo "[1/3] Installation des dépendances..."
if command -v zypper &> /dev/null; then
    zypper --non-interactive install python3-gobject python3-gobject-Gdk gtk3
elif command -v apt-get &> /dev/null; then
    apt-get install -y python3-gi python3-gi-cairo gir1.2-gtk-3.0
elif command -v dnf &> /dev/null; then
    dnf install -y python3-gobject gtk3
fi

# Copier le script GUI
echo "[2/3] Installation du script..."
cp minifs-gui.py /usr/local/bin/minifs-gui
chmod +x /usr/local/bin/minifs-gui

# Installer le fichier .desktop
echo "[3/3] Installation du raccourci..."
cp minifs.desktop /usr/share/applications/

echo ""
echo "✓ Installation terminée!"
echo ""
echo "Vous pouvez lancer MiniFS GUI depuis:"
echo "  - Le menu Applications > Système > MiniFS Manager"
echo "  - En tapant: minifs-gui"
echo ""

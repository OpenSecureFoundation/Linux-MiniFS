#!/bin/bash
# Script pour corriger automatiquement le chemin FUSE selon le système

echo "Détection de la configuration FUSE..."

# Chercher où est fuse.h
FUSE_H=$(find /usr/include -name "fuse.h" | grep -v linux | head -1)

if [ -z "$FUSE_H" ]; then
    echo "ERREUR: fuse.h non trouvé!"
    echo "Installez fuse3-devel:"
    echo "  sudo zypper install fuse3-devel"
    exit 1
fi

echo "Trouvé: $FUSE_H"

# Déterminer le chemin d'include
if [[ "$FUSE_H" == *"/fuse3/"* ]]; then
    INCLUDE_PATH="fuse3/fuse.h"
elif [[ "$FUSE_H" == *"/fuse/"* ]]; then
    INCLUDE_PATH="fuse/fuse.h"
else
    echo "ERREUR: Chemin FUSE non reconnu"
    exit 1
fi

echo "Chemin d'include: $INCLUDE_PATH"

# Corriger src/main.c
sed -i "s|#include <.*fuse\.h>|#include <$INCLUDE_PATH>|" src/main.c

echo "✓ Fichier src/main.c corrigé"
echo ""
echo "Vous pouvez maintenant compiler avec: make"

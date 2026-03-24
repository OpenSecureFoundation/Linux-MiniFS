#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "Ce script doit être exécuté en tant que root"
        print_info "Utilisez: sudo $0"
        exit 1
    fi
}

check_fuse() {
    print_info "Vérification de FUSE..."
    
    if pkg-config --exists fuse3 2>/dev/null; then
        print_success "FUSE3 détecté"
        return 0
    elif pkg-config --exists fuse 2>/dev/null; then
        print_success "FUSE détecté"
        return 0
    else
        print_warning "FUSE non détecté"
        return 1
    fi
}

install_dependencies() {
    print_info "Installation des dépendances..."
    
    zypper --non-interactive refresh
    
    # Essayer d'installer fuse3, sinon fuse
    if zypper --non-interactive install fuse3 fuse3-devel 2>/dev/null; then
        print_success "FUSE3 installé"
    elif zypper --non-interactive install fuse fuse-devel 2>/dev/null; then
        print_success "FUSE installé"
    else
        print_error "Impossible d'installer FUSE"
        exit 1
    fi
    
    zypper --non-interactive install gcc make pkg-config
    
    print_success "Dépendances installées"
}

compile_minifs() {
    print_info "Compilation de MiniFS..."
    
    # Afficher la configuration détectée
    make help | grep -A2 "Configuration"
    
    make clean
    
    if make; then
        print_success "Compilation réussie"
    else
        print_error "Échec de la compilation"
        print_info "Vérifiez les erreurs ci-dessus"
        exit 1
    fi
}

install_binaries() {
    print_info "Installation des binaires..."
    
    if make install; then
        print_success "Installation réussie"
    else
        print_error "Échec de l'installation"
        exit 1
    fi
}

setup_fuse_module() {
    print_info "Configuration du module FUSE..."
    
    if ! lsmod | grep -q fuse; then
        modprobe fuse || print_warning "Module FUSE non chargé"
    fi
    
    if [ ! -f /etc/modules-load.d/fuse.conf ]; then
        echo "fuse" > /etc/modules-load.d/fuse.conf
        print_info "Module FUSE configuré pour le démarrage"
    fi
    
    if [ ! -f /etc/fuse.conf ]; then
        touch /etc/fuse.conf
    fi
    
    if ! grep -q "user_allow_other" /etc/fuse.conf 2>/dev/null; then
        echo "user_allow_other" >> /etc/fuse.conf
        print_info "FUSE configuré pour montages utilisateur"
    fi
}

test_installation() {
    print_info "Test de l'installation..."
    
    if command -v mkfs.minifs &> /dev/null; then
        print_success "mkfs.minifs installé"
    else
        print_error "mkfs.minifs non trouvé"
        return 1
    fi
    
    if command -v minifs_fuse &> /dev/null; then
        print_success "minifs_fuse installé"
    else
        print_error "minifs_fuse non trouvé"
        return 1
    fi
    
    # Test rapide
    TEST_IMG="/tmp/minifs_test_$$.img"
    dd if=/dev/zero of="$TEST_IMG" bs=1M count=5 &>/dev/null
    
    if mkfs.minifs "$TEST_IMG" --force &>/dev/null; then
        print_success "Test de formatage OK"
        rm -f "$TEST_IMG"
    else
        print_error "Test de formatage échoué"
        rm -f "$TEST_IMG"
        return 1
    fi
}

print_final_info() {
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                                                        ║${NC}"
    echo -e "${GREEN}║       MiniFS Installation Terminée avec Succès!        ║${NC}"
    echo -e "${GREEN}║                                                        ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Commandes installées :"
    echo "  • mkfs.minifs     - Formater une partition"
    echo "  • minifs_fuse     - Monter le système de fichiers"
    echo "  • fsck.minifs     - Vérifier le système"
    echo "  • debugfs.minifs  - Débogage"
    echo ""
    echo "Test rapide :"
    echo "  dd if=/dev/zero of=test.img bs=1M count=50"
    echo "  mkfs.minifs test.img"
    echo "  mkdir -p /mnt/minifs"
    echo "  sudo minifs_fuse test.img /mnt/minifs -o default_permissions"
    echo "  echo 'Hello!' > /mnt/minifs/test.txt"
    echo "  cat /mnt/minifs/test.txt"
    echo "  sudo umount /mnt/minifs"
    echo ""
}

main() {
    echo ""
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║                                                        ║"
    echo "║          Installation de MiniFS                       ║"
    echo "║          Version 1.0.0                                ║"
    echo "║                                                        ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo ""
    
    check_root
    
    if ! check_fuse; then
        install_dependencies
    fi
    
    compile_minifs
    install_binaries
    setup_fuse_module
    
    if test_installation; then
        print_final_info
        exit 0
    else
        print_error "Certains tests ont échoué"
        exit 1
    fi
}

main "$@"

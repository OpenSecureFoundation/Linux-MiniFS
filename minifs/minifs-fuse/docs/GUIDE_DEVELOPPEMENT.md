# Guide de Développement MiniFS

## Table des matières

- [Configuration de l'Environnement](#configuration-de-lenvironnement)
- [IDE Recommandés](#ide-recommandés)
- [Outils de Développement](#outils-de-développement)
- [Workflow de Développement](#workflow-de-développement)
- [Debugging](#debugging)
- [Tests](#tests)
- [Contribution](#contribution)

---

## Configuration de l'Environnement

### Système d'exploitation

**OpenSUSE Tumbleweed** est le système recommandé pour le développement de MiniFS.

### Installation des dépendances

```bash
# Mise à jour du système
sudo zypper refresh
sudo zypper update

# Outils de compilation
sudo zypper install -y \
    gcc \
    gcc-c++ \
    make \
    cmake \
    automake \
    autoconf \
    libtool

# Bibliothèques FUSE
sudo zypper install -y \
    fuse3 \
    fuse3-devel

# Outils de développement
sudo zypper install -y \
    git \
    pkg-config \
    gdb \
    valgrind \
    strace \
    ltrace \
    cppcheck \
    clang \
    clang-tools

# Documentation et formatage
sudo zypper install -y \
    doxygen \
    graphviz \
    clang-format \
    astyle
```

---

## IDE Recommandés

### 1. Visual Studio Code (Recommandé)

**Pourquoi VS Code ?**
- Gratuit et open-source
- Excellent support C/C++
- Extensions puissantes
- Débogage intégré
- Git intégré

#### Installation

```bash
# Ajouter le dépôt Microsoft
sudo rpm --import https://packages.microsoft.com/keys/microsoft.asc
sudo zypper addrepo https://packages.microsoft.com/yumrepos/vscode vscode

# Installer
sudo zypper refresh
sudo zypper install code
```

#### Extensions recommandées

```bash
# Lancer VS Code
code

# Installer les extensions (commande ou via l'interface)
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cpptools-extension-pack
code --install-extension ms-vscode.cmake-tools
code --install-extension ms-vscode.makefile-tools
code --install-extension streetsidesoftware.code-spell-checker
code --install-extension eamodio.gitlens
code --install-extension oderwat.indent-rainbow
code --install-extension christian-kohler.path-intellisense
```

#### Configuration VS Code

Créer `.vscode/settings.json` dans le projet :

```json
{
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/include",
        "/usr/include/fuse3",
        "/usr/include"
    ],
    "C_Cpp.default.defines": [
        "FUSE_USE_VERSION=35",
        "_FILE_OFFSET_BITS=64"
    ],
    "C_Cpp.default.compilerPath": "/usr/bin/gcc",
    "C_Cpp.default.cStandard": "c11",
    "C_Cpp.default.cppStandard": "c++17",
    "C_Cpp.default.intelliSenseMode": "linux-gcc-x64",
    
    "files.associations": {
        "*.h": "c",
        "*.c": "c"
    },
    
    "editor.formatOnSave": true,
    "editor.tabSize": 4,
    "editor.insertSpaces": true,
    "editor.rulers": [80, 100],
    
    "C_Cpp.clang_format_style": "{ BasedOnStyle: LLVM, IndentWidth: 4, UseTab: Never, BreakBeforeBraces: Linux }",
    
    "files.exclude": {
        "**/.git": true,
        "**/build": true,
        "**/bin": true,
        "**/*.o": true
    }
}
```

Créer `.vscode/c_cpp_properties.json` :

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "/usr/include/fuse3",
                "/usr/include"
            ],
            "defines": [
                "FUSE_USE_VERSION=35",
                "_FILE_OFFSET_BITS=64"
            ],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

Créer `.vscode/tasks.json` :

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build MiniFS",
            "type": "shell",
            "command": "make",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Clean",
            "type": "shell",
            "command": "make clean"
        },
        {
            "label": "Install",
            "type": "shell",
            "command": "sudo make install"
        },
        {
            "label": "Build Debug",
            "type": "shell",
            "command": "make DEBUG=1"
        },
        {
            "label": "Run Tests",
            "type": "shell",
            "command": "./tests/test_suite.sh"
        }
    ]
}
```

Créer `.vscode/launch.json` pour le debugging :

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug MiniFS",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/bin/minifs_fuse",
            "args": ["-d", "-f", "test.img", "/tmp/minifs_mount"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build Debug",
            "miDebuggerPath": "/usr/bin/gdb"
        },
        {
            "name": "Debug mkfs.minifs",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/bin/mkfs.minifs",
            "args": ["test.img"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "MIMode": "gdb"
        }
    ]
}
```

---

### 2. CLion (Alternative Professionnelle)

**Pourquoi CLion ?**
- IDE complet pour C/C++
- Refactoring puissant
- Analyse de code avancée
- Débogage sophistiqué

#### Installation

```bash
# Télécharger depuis JetBrains
# Ou via Snap
sudo snap install clion --classic
```

#### Configuration CLion

1. **Ouvrir le projet** : `File > Open > /path/to/minifs-fuse`
2. **Configurer CMake** (si vous créez un CMakeLists.txt)
3. **Configurer le compilateur** : `Settings > Build > Toolchains`
4. **Configurer le débogueur** : GDB est détecté automatiquement

---

### 3. Code::Blocks (Alternative Légère)

**Pourquoi Code::Blocks ?**
- Léger et rapide
- Interface simple
- Bon pour petits projets

#### Installation

```bash
sudo zypper install codeblocks codeblocks-contrib
```

---

## Outils de Développement

### 1. GDB - Débogueur GNU

#### Installation
```bash
sudo zypper install gdb
```

#### Utilisation de base

```bash
# Compiler avec symboles de débogage
make DEBUG=1

# Lancer GDB
gdb ./bin/minifs_fuse

# Commandes GDB essentielles
(gdb) break main              # Point d'arrêt à main
(gdb) break filesystem.c:42   # Point d'arrêt ligne 42
(gdb) run test.img /mnt       # Lancer le programme
(gdb) next                    # Ligne suivante
(gdb) step                    # Entrer dans fonction
(gdb) print variable          # Afficher variable
(gdb) backtrace               # Pile d'appels
(gdb) continue                # Continuer exécution
(gdb) quit                    # Quitter
```

#### Fichier .gdbinit recommandé

Créer `~/.gdbinit` :

```gdb
# Pretty printing
set print pretty on
set print array on
set print array-indexes on

# History
set history save on
set history size 10000
set history filename ~/.gdb_history

# Display
set pagination off
set confirm off

# Auto-load safe path
add-auto-load-safe-path /home/your_username/minifs-fuse
```

---

### 2. Valgrind - Détection de Fuites Mémoire

#### Installation
```bash
sudo zypper install valgrind
```

#### Utilisation

```bash
# Vérification basique
valgrind --leak-check=yes ./bin/minifs_fuse test.img /mnt

# Vérification complète
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind.log \
         ./bin/minifs_fuse test.img /mnt

# Analyser le rapport
less valgrind.log
```

---

### 3. Strace - Traçage d'Appels Système

#### Utilisation

```bash
# Tracer tous les appels système
strace ./bin/minifs_fuse test.img /mnt 2>&1 | tee strace.log

# Tracer uniquement les appels de fichiers
strace -e trace=file ./bin/minifs_fuse test.img /mnt

# Compter les appels
strace -c ./bin/minifs_fuse test.img /mnt

# Tracer un processus existant
strace -p <PID>
```

---

### 4. CppCheck - Analyse Statique

#### Installation
```bash
sudo zypper install cppcheck
```

#### Utilisation

```bash
# Vérifier tout le code source
cppcheck --enable=all --inconclusive --std=c11 src/

# Générer un rapport XML
cppcheck --enable=all --xml --xml-version=2 src/ 2> cppcheck.xml

# Vérifier avec includes
cppcheck --enable=all -I include/ src/
```

---

### 5. Clang-Format - Formatage de Code

#### Installation
```bash
sudo zypper install clang-format
```

#### Configuration

Créer `.clang-format` à la racine du projet :

```yaml
---
Language: Cpp
BasedOnStyle: LLVM
IndentWidth: 4
UseTab: Never
BreakBeforeBraces: Linux
AllowShortIfStatementsOnASingleLine: false
IndentCaseLabels: false
ColumnLimit: 100
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignTrailingComments: true
AllowShortFunctionsOnASingleLine: None
AllowShortBlocksOnASingleLine: false
KeepEmptyLinesAtTheStartOfBlocks: false
MaxEmptyLinesToKeep: 1
PointerAlignment: Right
SpaceAfterCStyleCast: false
SpaceBeforeParens: ControlStatements
SpacesInParentheses: false
SpacesInSquareBrackets: false
```

#### Utilisation

```bash
# Formater un fichier
clang-format -i src/filesystem.c

# Formater tous les fichiers
find src/ -name "*.c" -o -name "*.h" | xargs clang-format -i

# Vérifier sans modifier
clang-format --dry-run --Werror src/filesystem.c
```

---

### 6. Doxygen - Documentation

#### Installation
```bash
sudo zypper install doxygen graphviz
```

#### Configuration

Créer `Doxyfile` :

```bash
doxygen -g Doxyfile
```

Modifier les paramètres :

```
PROJECT_NAME = "MiniFS"
PROJECT_BRIEF = "Minimal Filesystem with FUSE"
OUTPUT_DIRECTORY = docs/doxygen
INPUT = src include
RECURSIVE = YES
EXTRACT_ALL = YES
EXTRACT_PRIVATE = YES
EXTRACT_STATIC = YES
GENERATE_HTML = YES
GENERATE_LATEX = NO
HAVE_DOT = YES
CALL_GRAPH = YES
CALLER_GRAPH = YES
```

#### Génération de la documentation

```bash
doxygen Doxyfile

# Ouvrir la documentation
firefox docs/doxygen/html/index.html
```

---

## Workflow de Développement

### 1. Cloner et Configuration Initiale

```bash
# Cloner le projet
git clone https://github.com/your-repo/minifs-fuse.git
cd minifs-fuse

# Créer une branche de développement
git checkout -b feature/my-feature

# Compiler
make clean
make DEBUG=1
```

### 2. Développement Itératif

```bash
# Éditer le code
vim src/filesystem.c

# Compiler
make

# Tester
./tests/test_suite.sh

# Si erreurs, déboguer
gdb ./bin/minifs_fuse
```

### 3. Vérification de la Qualité

```bash
# Analyse statique
cppcheck --enable=all src/

# Vérification mémoire
valgrind --leak-check=full ./bin/mkfs.minifs test.img

# Formatage
find src/ -name "*.c" | xargs clang-format -i

# Tests complets
make test
```

### 4. Commit et Push

```bash
# Ajouter les fichiers
git add src/filesystem.c

# Commit avec message descriptif
git commit -m "Add: Implement fragmentation detection in filesystem.c"

# Push
git push origin feature/my-feature
```

---

## Debugging

### Techniques de Debugging

#### 1. Logs de Débogage

Ajouter dans le code :

```c
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "[DEBUG] %s:%d:%s(): " fmt "\n", \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) do {} while(0)
#endif

// Utilisation
DEBUG_PRINT("Allocating inode %u", inode_num);
```

#### 2. Debugging avec GDB - Scénario Complet

```bash
# 1. Compiler en mode debug
make clean && make DEBUG=1

# 2. Créer une image de test
dd if=/dev/zero of=debug.img bs=1M count=10
./bin/mkfs.minifs debug.img

# 3. Lancer GDB
gdb --args ./bin/minifs_fuse -d -f debug.img /tmp/test_mount

# 4. Dans GDB
(gdb) break alloc_inode
(gdb) run

# 5. Dans un autre terminal, déclencher l'opération
touch /tmp/test_mount/testfile.txt

# 6. Examiner l'état dans GDB
(gdb) print context->sb.free_inodes
(gdb) print *inode
(gdb) backtrace
(gdb) continue
```

#### 3. Core Dumps

Activer les core dumps :

```bash
# Autoriser les core dumps
ulimit -c unlimited

# Configurer le pattern
echo "core.%e.%p" | sudo tee /proc/sys/kernel/core_pattern

# Si crash, analyser
gdb ./bin/minifs_fuse core.minifs_fuse.12345
(gdb) backtrace
(gdb) print variable
```

---

## Tests

### Structure des Tests

```
tests/
├── test_suite.sh           # Suite complète
├── unit/
│   ├── test_bitmap.sh
│   ├── test_inode.sh
│   └── test_block.sh
├── integration/
│   ├── test_create.sh
│   ├── test_read_write.sh
│   └── test_fragmentation.sh
└── stress/
    └── test_concurrent.sh
```

### Exécution des Tests

```bash
# Tous les tests
./tests/test_suite.sh

# Tests unitaires seulement
./tests/unit/*.sh

# Test spécifique
./tests/unit/test_inode.sh

# Tests avec couverture (si configuré)
make coverage
```

### Écrire un Nouveau Test

Template `tests/unit/test_example.sh` :

```bash
#!/bin/bash

# Test configuration
TEST_NAME="Example Test"
TEST_IMG="test_example.img"
TEST_MOUNT="/tmp/minifs_test_example"

# Setup
setup() {
    echo "=== Setting up $TEST_NAME ==="
    dd if=/dev/zero of=$TEST_IMG bs=1M count=10 &>/dev/null
    mkfs.minifs $TEST_IMG &>/dev/null
    mkdir -p $TEST_MOUNT
    mount.minifs $TEST_IMG $TEST_MOUNT
}

# Cleanup
cleanup() {
    umount $TEST_MOUNT 2>/dev/null
    rm -rf $TEST_IMG $TEST_MOUNT
}

# Test case
test_example_operation() {
    echo "  Testing example operation..."
    
    # Perform operation
    echo "test data" > $TEST_MOUNT/testfile.txt
    
    # Verify
    content=$(cat $TEST_MOUNT/testfile.txt)
    if [ "$content" == "test data" ]; then
        echo "  ✓ PASS"
        return 0
    else
        echo "  ✗ FAIL: Expected 'test data', got '$content'"
        return 1
    fi
}

# Run tests
setup
test_example_operation
result=$?
cleanup
exit $result
```

---

## Contribution

### Workflow de Contribution

1. **Fork** le projet
2. **Créer une branche** : `git checkout -b feature/amazing-feature`
3. **Commit** les changements : `git commit -m 'Add amazing feature'`
4. **Push** vers la branche : `git push origin feature/amazing-feature`
5. **Ouvrir une Pull Request**

### Standards de Code

- **Style** : Suivre la convention Linux Kernel pour C
- **Documentation** : Commenter les fonctions complexes
- **Tests** : Ajouter des tests pour les nouvelles fonctionnalités
- **Commits** : Messages descriptifs et atomiques

### Checklist avant Pull Request

- [ ] Code compilé sans warnings
- [ ] Tests passent
- [ ] Code formaté (clang-format)
- [ ] Pas de fuites mémoire (valgrind)
- [ ] Documentation mise à jour
- [ ] CHANGELOG.md mis à jour

---

## Ressources Supplémentaires

### Documentation

- [FUSE Documentation](https://libfuse.github.io/doxygen/)
- [Linux VFS](https://www.kernel.org/doc/html/latest/filesystems/vfs.html)
- [GDB Manual](https://sourceware.org/gdb/current/onlinedocs/gdb/)

### Communauté

- GitHub Issues
- Stack Overflow (tag: fuse, filesystem)
- IRC: #fuse on irc.freenode.net

---

**Dernière mise à jour :** Mars 2025  
**Mainteneur :** Projet MiniFS

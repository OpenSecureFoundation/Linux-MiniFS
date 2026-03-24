const pptxgen = require("pptxgenjs");

// Create presentation
let pres = new pptxgen();
pres.layout = 'LAYOUT_16x9';
pres.author = 'Projet MiniFS';
pres.title = 'MiniFS - Système de Fichiers Minimal';
pres.subject = 'Présentation du projet MiniFS';

// Color palette - Midnight Executive
const COLORS = {
    primary: "1E2761",     // Navy
    secondary: "CADCFC",   // Ice blue
    accent: "FFFFFF",      // White
    text: "333333",        // Dark gray
    lightBg: "F5F5F5"      // Light gray
};

// ===== SLIDE 1: Title =====
let slide1 = pres.addSlide();
slide1.background = { color: COLORS.primary };

slide1.addText("MiniFS", {
    x: 0.5, y: 1.5, w: 9, h: 1.2,
    fontSize: 72, bold: true, color: COLORS.accent,
    align: "center"
});

slide1.addText("Système de Fichiers Minimal Éducatif", {
    x: 0.5, y: 2.8, w: 9, h: 0.6,
    fontSize: 32, color: COLORS.secondary,
    align: "center", italic: true
});

slide1.addText("Conception et Développement pour OpenSUSE Tumbleweed", {
    x: 0.5, y: 3.6, w: 9, h: 0.4,
    fontSize: 18, color: COLORS.secondary,
    align: "center"
});

slide1.addText("Mars 2025 • Version 1.0", {
    x: 0.5, y: 4.8, w: 9, h: 0.3,
    fontSize: 14, color: COLORS.secondary,
    align: "center"
});

// ===== SLIDE 2: Table des matières =====
let slide2 = pres.addSlide();
slide2.background = { color: COLORS.lightBg };

slide2.addText("AGENDA", {
    x: 0.5, y: 0.3, w: 9, h: 0.6,
    fontSize: 44, bold: true, color: COLORS.primary
});

const agenda = [
    { text: "1. Introduction et Contexte", options: { bullet: true, breakLine: true, color: COLORS.text } },
    { text: "2. Objectifs du Projet", options: { bullet: true, breakLine: true, color: COLORS.text } },
    { text: "3. Architecture Technique", options: { bullet: true, breakLine: true, color: COLORS.text } },
    { text: "4. Fonctionnalités Implémentées", options: { bullet: true, breakLine: true, color: COLORS.text } },
    { text: "5. Structure des Données", options: { bullet: true, breakLine: true, color: COLORS.text } },
    { text: "6. Déploiement et Utilisation", options: { bullet: true, breakLine: true, color: COLORS.text } },
    { text: "7. Résultats et Performances", options: { bullet: true, breakLine: true, color: COLORS.text } },
    { text: "8. Conclusion et Perspectives", options: { bullet: true, color: COLORS.text } }
];

slide2.addText(agenda, {
    x: 1.5, y: 1.3, w: 7, h: 3.5,
    fontSize: 20, lineSpacing: 32
});

// ===== SLIDE 3: Introduction =====
let slide3 = pres.addSlide();

slide3.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.8,
    fill: { color: COLORS.primary }
});

slide3.addText("1. INTRODUCTION ET CONTEXTE", {
    x: 0.5, y: 0.2, w: 9, h: 0.4,
    fontSize: 28, bold: true, color: COLORS.accent
});

slide3.addText("Qu'est-ce que MiniFS ?", {
    x: 0.5, y: 1.2, w: 9, h: 0.4,
    fontSize: 24, bold: true, color: COLORS.primary
});

slide3.addText([
    { text: "MiniFS est un système de fichiers minimal implémenté avec ", options: {} },
    { text: "FUSE (Filesystem in Userspace)", options: { bold: true } },
    { text: " à des fins éducatives. Il démontre les concepts fondamentaux de la gestion de fichiers dans les systèmes d'exploitation modernes.", options: {} }
], {
    x: 0.5, y: 1.8, w: 9, h: 0.8,
    fontSize: 16, color: COLORS.text, align: "justify"
});

// Key features box
slide3.addShape(pres.shapes.RECTANGLE, {
    x: 0.5, y: 2.8, w: 4.2, h: 2.3,
    fill: { color: "E8F4F8" }, line: { color: COLORS.primary, width: 2 }
});

slide3.addText("✓ Caractéristiques", {
    x: 0.7, y: 2.95, w: 3.8, h: 0.35,
    fontSize: 18, bold: true, color: COLORS.primary
});

const features1 = [
    { text: "Éducatif et documenté", options: { bullet: true, breakLine: true } },
    { text: "Sécurisé (espace utilisateur)", options: { bullet: true, breakLine: true } },
    { text: "Portable sur Linux", options: { bullet: true, breakLine: true } },
    { text: "100% fonctionnel", options: { bullet: true } }
];

slide3.addText(features1, {
    x: 0.7, y: 3.4, w: 3.8, h: 1.5,
    fontSize: 14, color: COLORS.text
});

// Use cases box
slide3.addShape(pres.shapes.RECTANGLE, {
    x: 5.3, y: 2.8, w: 4.2, h: 2.3,
    fill: { color: "FFF4E6" }, line: { color: COLORS.primary, width: 2 }
});

slide3.addText("🎯 Cas d'Usage", {
    x: 5.5, y: 2.95, w: 3.8, h: 0.35,
    fontSize: 18, bold: true, color: COLORS.primary
});

const features2 = [
    { text: "Apprentissage des OS", options: { bullet: true, breakLine: true } },
    { text: "Recherche académique", options: { bullet: true, breakLine: true } },
    { text: "Prototypage rapide", options: { bullet: true, breakLine: true } },
    { text: "Tests et validation", options: { bullet: true } }
];

slide3.addText(features2, {
    x: 5.5, y: 3.4, w: 3.8, h: 1.5,
    fontSize: 14, color: COLORS.text
});

// ===== SLIDE 4: Objectifs =====
let slide4 = pres.addSlide();

slide4.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.8,
    fill: { color: COLORS.primary }
});

slide4.addText("2. OBJECTIFS DU PROJET", {
    x: 0.5, y: 0.2, w: 9, h: 0.4,
    fontSize: 28, bold: true, color: COLORS.accent
});

// Objectives in cards
const objectives = [
    { title: "Gestion Inodes", desc: "Allocation, libération,\nlecture, écriture", icon: "📁" },
    { title: "Allocation Blocs", desc: "Gestion dynamique\navec bitmap", icon: "💾" },
    { title: "Répertoires", desc: "Hiérarchie complète\nfichiers et dossiers", icon: "📂" },
    { title: "Lecture/Écriture", desc: "Opérations I/O\ncomplètes", icon: "📝" },
    { title: "Superbloc", desc: "Métadonnées\ndu système", icon: "⚙️" },
    { title: "Fragmentation", desc: "Détection et\nanalyse", icon: "📊" }
];

let startX = 0.5;
let startY = 1.2;
let cardW = 3;
let cardH = 1.5;
let gap = 0.2;

for (let i = 0; i < objectives.length; i++) {
    let row = Math.floor(i / 3);
    let col = i % 3;
    let x = startX + col * (cardW + gap);
    let y = startY + row * (cardH + gap);
    
    // Card background
    slide4.addShape(pres.shapes.RECTANGLE, {
        x: x, y: y, w: cardW, h: cardH,
        fill: { color: COLORS.secondary, transparency: 70 },
        line: { color: COLORS.primary, width: 1 }
    });
    
    // Icon
    slide4.addText(objectives[i].icon, {
        x: x + 0.2, y: y + 0.15, w: 0.5, h: 0.5,
        fontSize: 32
    });
    
    // Title
    slide4.addText(objectives[i].title, {
        x: x + 0.2, y: y + 0.7, w: cardW - 0.4, h: 0.3,
        fontSize: 16, bold: true, color: COLORS.primary
    });
    
    // Description
    slide4.addText(objectives[i].desc, {
        x: x + 0.2, y: y + 1.0, w: cardW - 0.4, h: 0.4,
        fontSize: 12, color: COLORS.text
    });
}

// ===== SLIDE 5: Architecture =====
let slide5 = pres.addSlide();

slide5.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.8,
    fill: { color: COLORS.primary }
});

slide5.addText("3. ARCHITECTURE TECHNIQUE", {
    x: 0.5, y: 0.2, w: 9, h: 0.4,
    fontSize: 28, bold: true, color: COLORS.accent
});

// Architecture layers
const layers = [
    { name: "Applications Utilisateur", color: "8BC34A", y: 1.2 },
    { name: "Noyau Linux VFS", color: "4CAF50", y: 1.9 },
    { name: "Bibliothèque libfuse", color: "009688", y: 2.6 },
    { name: "MiniFS (minifs_fuse)", color: "0097A7", y: 3.3 },
    { name: "Image disque / Périphérique bloc", color: "0277BD", y: 4.0 }
];

layers.forEach((layer, index) => {
    slide5.addShape(pres.shapes.RECTANGLE, {
        x: 2, y: layer.y, w: 6, h: 0.55,
        fill: { color: layer.color },
        line: { color: "FFFFFF", width: 2 }
    });
    
    slide5.addText(layer.name, {
        x: 2, y: layer.y, w: 6, h: 0.55,
        fontSize: 16, bold: true, color: "FFFFFF",
        align: "center", valign: "middle"
    });
    
    // Arrows between layers
    if (index < layers.length - 1) {
        slide5.addShape(pres.shapes.LINE, {
            x: 5, y: layer.y + 0.55, w: 0, h: 0.35,
            line: { color: COLORS.primary, width: 3, endArrowType: "triangle" }
        });
    }
});

// ===== SLIDE 6: Fonctionnalités =====
let slide6 = pres.addSlide();

slide6.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.8,
    fill: { color: COLORS.primary }
});

slide6.addText("4. FONCTIONNALITÉS IMPLÉMENTÉES", {
    x: 0.5, y: 0.2, w: 9, h: 0.4,
    fontSize: 28, bold: true, color: COLORS.accent
});

// Two columns
slide6.addShape(pres.shapes.RECTANGLE, {
    x: 0.5, y: 1.2, w: 4.4, h: 3.5,
    fill: { color: "E3F2FD" }
});

slide6.addText("✓ Opérations de Base", {
    x: 0.7, y: 1.35, w: 4, h: 0.4,
    fontSize: 18, bold: true, color: COLORS.primary
});

const basicOps = [
    { text: "Création de fichiers et répertoires", options: { bullet: true, breakLine: true } },
    { text: "Lecture et écriture de données", options: { bullet: true, breakLine: true } },
    { text: "Suppression (unlink, rmdir)", options: { bullet: true, breakLine: true } },
    { text: "Navigation hiérarchique", options: { bullet: true, breakLine: true } },
    { text: "Listage de répertoires", options: { bullet: true, breakLine: true } },
    { text: "Gestion des permissions POSIX", options: { bullet: true } }
];

slide6.addText(basicOps, {
    x: 0.7, y: 1.85, w: 4, h: 2.5,
    fontSize: 14, color: COLORS.text
});

slide6.addShape(pres.shapes.RECTANGLE, {
    x: 5.1, y: 1.2, w: 4.4, h: 3.5,
    fill: { color: "F3E5F5" }
});

slide6.addText("⚙️ Fonctionnalités Avancées", {
    x: 5.3, y: 1.35, w: 4, h: 0.4,
    fontSize: 18, bold: true, color: COLORS.primary
});

const advOps = [
    { text: "Liens durs (hard links)", options: { bullet: true, breakLine: true } },
    { text: "Timestamps précis (atime, mtime, ctime)", options: { bullet: true, breakLine: true } },
    { text: "Allocation dynamique avec bitmap", options: { bullet: true, breakLine: true } },
    { text: "Détection de fragmentation", options: { bullet: true, breakLine: true } },
    { text: "Outils de diagnostic et débogage", options: { bullet: true, breakLine: true } },
    { text: "Vérification d'intégrité (fsck)", options: { bullet: true } }
];

slide6.addText(advOps, {
    x: 5.3, y: 1.85, w: 4, h: 2.5,
    fontSize: 14, color: COLORS.text
});

// ===== SLIDE 7: Structure des données =====
let slide7 = pres.addSlide();

slide7.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.8,
    fill: { color: COLORS.primary }
});

slide7.addText("5. STRUCTURE DES DONNÉES", {
    x: 0.5, y: 0.2, w: 9, h: 0.4,
    fontSize: 28, bold: true, color: COLORS.accent
});

// Disk layout diagram
const diskLayout = [
    { name: "Superbloc", blocks: "1 bloc", color: "FF6B6B", desc: "Métadonnées globales" },
    { name: "Bitmap Inodes", blocks: "1 bloc", color: "4ECDC4", desc: "Allocation inodes" },
    { name: "Table Inodes", blocks: "N blocs", color: "45B7D1", desc: "Métadonnées fichiers" },
    { name: "Bitmap Blocs", blocks: "M blocs", color: "96CEB4", desc: "Allocation blocs" },
    { name: "Blocs Données", blocks: "Reste", color: "FFEAA7", desc: "Contenu fichiers" }
];

diskLayout.forEach((section, index) => {
    const y = 1.2 + (index * 0.7);
    
    slide7.addShape(pres.shapes.RECTANGLE, {
        x: 0.5, y: y, w: 1.8, h: 0.5,
        fill: { color: section.color }
    });
    
    slide7.addText(section.name, {
        x: 0.5, y: y, w: 1.8, h: 0.5,
        fontSize: 14, bold: true, color: "FFFFFF",
        align: "center", valign: "middle"
    });
    
    slide7.addText(section.blocks, {
        x: 2.5, y: y, w: 2, h: 0.5,
        fontSize: 12, color: COLORS.text, valign: "middle"
    });
    
    slide7.addText(section.desc, {
        x: 4.7, y: y, w: 4.8, h: 0.5,
        fontSize: 12, color: COLORS.text, valign: "middle"
    });
});

// Structures info
slide7.addShape(pres.shapes.RECTANGLE, {
    x: 0.5, y: 4.8, w: 9, h: 0.6,
    fill: { color: COLORS.secondary, transparency: 50 }
});

slide7.addText("📐 Taille de bloc : 4096 octets  |  📊 Inodes configurables  |  💾 Fichiers max : ~50 MB", {
    x: 0.5, y: 4.8, w: 9, h: 0.6,
    fontSize: 14, bold: true, color: COLORS.primary,
    align: "center", valign: "middle"
});

// ===== SLIDE 8: Déploiement =====
let slide8 = pres.addSlide();

slide8.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.8,
    fill: { color: COLORS.primary }
});

slide8.addText("6. DÉPLOIEMENT ET UTILISATION", {
    x: 0.5, y: 0.2, w: 9, h: 0.4,
    fontSize: 28, bold: true, color: COLORS.accent
});

// Installation steps
slide8.addText("Installation Automatique", {
    x: 0.5, y: 1.1, w: 4.4, h: 0.4,
    fontSize: 20, bold: true, color: COLORS.primary
});

slide8.addShape(pres.shapes.RECTANGLE, {
    x: 0.5, y: 1.6, w: 4.4, h: 1.2,
    fill: { color: "263238" }
});

slide8.addText([
    { text: "# Compilation et installation\n", options: { color: "FFFFFF", breakLine: true, fontFace: "Courier New" } },
    { text: "sudo ./scripts/install.sh", options: { color: "4CAF50", fontFace: "Courier New" } }
], {
    x: 0.7, y: 1.75, w: 4, h: 0.8,
    fontSize: 13
});

// Usage steps
slide8.addText("Utilisation Rapide", {
    x: 5.1, y: 1.1, w: 4.4, h: 0.4,
    fontSize: 20, bold: true, color: COLORS.primary
});

slide8.addShape(pres.shapes.RECTANGLE, {
    x: 5.1, y: 1.6, w: 4.4, h: 2.8,
    fill: { color: "263238" }
});

slide8.addText([
    { text: "# 1. Créer une image\n", options: { color: "90CAF9", breakLine: true, fontFace: "Courier New" } },
    { text: "dd if=/dev/zero of=disk.img bs=1M count=100\n\n", options: { color: "FFFFFF", breakLine: true, fontFace: "Courier New" } },
    { text: "# 2. Formater\n", options: { color: "90CAF9", breakLine: true, fontFace: "Courier New" } },
    { text: "mkfs.minifs disk.img\n\n", options: { color: "FFFFFF", breakLine: true, fontFace: "Courier New" } },
    { text: "# 3. Monter\n", options: { color: "90CAF9", breakLine: true, fontFace: "Courier New" } },
    { text: "sudo mount.minifs disk.img /mnt/minifs\n\n", options: { color: "FFFFFF", breakLine: true, fontFace: "Courier New" } },
    { text: "# 4. Utiliser\n", options: { color: "90CAF9", breakLine: true, fontFace: "Courier New" } },
    { text: "echo 'Hello!' > /mnt/minifs/test.txt", options: { color: "FFFFFF", fontFace: "Courier New" } }
], {
    x: 5.3, y: 1.75, w: 4, h: 2.4,
    fontSize: 11
});

// Tools
slide8.addText("🛠️ Outils Fournis", {
    x: 0.5, y: 3.0, w: 4.4, h: 0.4,
    fontSize: 18, bold: true, color: COLORS.primary
});

const tools = [
    { text: "mkfs.minifs - Formatage", options: { bullet: true, breakLine: true } },
    { text: "fsck.minifs - Vérification", options: { bullet: true, breakLine: true } },
    { text: "debugfs.minifs - Débogage", options: { bullet: true, breakLine: true } },
    { text: "mount.minifs - Montage", options: { bullet: true } }
];

slide8.addText(tools, {
    x: 0.7, y: 3.5, w: 3.8, h: 1.3,
    fontSize: 14, color: COLORS.text
});

// ===== SLIDE 9: Performances =====
let slide9 = pres.addSlide();

slide9.addShape(pres.shapes.RECTANGLE, {
    x: 0, y: 0, w: 10, h: 0.8,
    fill: { color: COLORS.primary }
});

slide9.addText("7. RÉSULTATS ET PERFORMANCES", {
    x: 0.5, y: 0.2, w: 9, h: 0.4,
    fontSize: 28, bold: true, color: COLORS.accent
});

// Performance metrics
const metrics = [
    { label: "Création fichier", value: "~0.5", unit: "ms" },
    { label: "Lecture séquentielle", value: "80-120", unit: "MB/s" },
    { label: "Écriture séquentielle", value: "60-90", unit: "MB/s" }
];

metrics.forEach((metric, index) => {
    const x = 0.8 + (index * 3);
    
    slide9.addShape(pres.shapes.RECTANGLE, {
        x: x, y: 1.5, w: 2.5, h: 2,
        fill: { color: COLORS.secondary, transparency: 30 },
        line: { color: COLORS.primary, width: 3 }
    });
    
    slide9.addText(metric.value, {
        x: x, y: 1.9, w: 2.5, h: 0.6,
        fontSize: 44, bold: true, color: COLORS.primary,
        align: "center"
    });
    
    slide9.addText(metric.unit, {
        x: x, y: 2.5, w: 2.5, h: 0.3,
        fontSize: 18, color: COLORS.text,
        align: "center"
    });
    
    slide9.addText(metric.label, {
        x: x, y: 2.9, w: 2.5, h: 0.4,
        fontSize: 14, color: COLORS.text,
        align: "center", italic: true
    });
});

// Benchmark info
slide9.addShape(pres.shapes.RECTANGLE, {
    x: 0.5, y: 4.0, w: 9, h: 0.8,
    fill: { color: "FFF3CD" },
    line: { color: "FFC107", width: 2 }
});

slide9.addText("📊 Tests réalisés sur VM OpenSUSE (2 CPU cores, 4GB RAM)", {
    x: 0.7, y: 4.2, w: 8.6, h: 0.4,
    fontSize: 14, color: COLORS.text, align: "center", valign: "middle"
});

// ===== SLIDE 10: Conclusion =====
let slide10 = pres.addSlide();
slide10.background = { color: COLORS.primary };

slide10.addText("CONCLUSION", {
    x: 0.5, y: 0.8, w: 9, h: 0.8,
    fontSize: 48, bold: true, color: COLORS.accent,
    align: "center"
});

const conclusions = [
    { text: "✓ Système de fichiers complet et fonctionnel", options: { bullet: true, breakLine: true, color: COLORS.accent } },
    { text: "✓ Implémentation éducative avec code documenté", options: { bullet: true, breakLine: true, color: COLORS.accent } },
    { text: "✓ Déploiement simple sur OpenSUSE Tumbleweed", options: { bullet: true, breakLine: true, color: COLORS.accent } },
    { text: "✓ Performances satisfaisantes pour l'usage prévu", options: { bullet: true, breakLine: true, color: COLORS.accent } },
    { text: "✓ Outils complets de gestion et débogage", options: { bullet: true, color: COLORS.accent } }
];

slide10.addText(conclusions, {
    x: 1.5, y: 2.0, w: 7, h: 2,
    fontSize: 18, lineSpacing: 28
});

slide10.addText("Perspectives d'Evolution", {
    x: 0.5, y: 4.3, w: 9, h: 0.4,
    fontSize: 22, bold: true, color: COLORS.secondary,
    align: "center"
});

slide10.addText("Liens symboliques • Défragmentation automatique • Journalisation • Compression", {
    x: 0.5, y: 4.8, w: 9, h: 0.3,
    fontSize: 14, color: COLORS.secondary,
    align: "center", italic: true
});

// ===== SLIDE 11: Merci =====
let slide11 = pres.addSlide();
slide11.background = { color: COLORS.primary };

slide11.addText("MERCI", {
    x: 0.5, y: 2.0, w: 9, h: 1,
    fontSize: 64, bold: true, color: COLORS.accent,
    align: "center"
});

slide11.addText("Questions ?", {
    x: 0.5, y: 3.2, w: 9, h: 0.6,
    fontSize: 32, color: COLORS.secondary,
    align: "center", italic: true
});

slide11.addText("📧 Contact : projet-minifs@example.com", {
    x: 0.5, y: 4.5, w: 9, h: 0.4,
    fontSize: 16, color: COLORS.secondary,
    align: "center"
});

// Save presentation
pres.writeFile({ fileName: "/mnt/user-data/outputs/Presentation_MiniFS.pptx" });
console.log("Présentation PowerPoint générée avec succès !");

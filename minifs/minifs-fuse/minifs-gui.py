#!/usr/bin/env python3
"""
MiniFS GUI Amélioré - Interface avec montage direct et options séparées
"""

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib, Pango
import subprocess
import os
import threading
import time

class MinifsGUI(Gtk.Window):
    def __init__(self):
        super().__init__(title="MiniFS - Gestionnaire de Système de Fichiers")
        self.set_border_width(10)
        self.set_default_size(950, 750)
        
        # Variables
        self.current_image = None
        self.current_mountpoint = None
        self.mount_process = None
        self.is_mounted = False
        
        # Trouver les binaires
        self.mkfs_path = self.find_binary("mkfs.minifs")
        self.mount_path = self.find_binary("minifs_fuse")
        self.fsck_path = self.find_binary("fsck.minifs")
        self.debugfs_path = self.find_binary("debugfs.minifs")
        
        # Layout principal
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        self.add(vbox)
        
        # Barre de menu
        menubar = self.create_menubar()
        vbox.pack_start(menubar, False, False, 0)
        
        # Barre d'outils
        toolbar = self.create_toolbar()
        vbox.pack_start(toolbar, False, False, 0)
        
        # Notebook
        notebook = Gtk.Notebook()
        vbox.pack_start(notebook, True, True, 0)
        
        # Onglets
        page1 = self.create_format_page()
        notebook.append_page(page1, Gtk.Label(label="Créer & Formater"))
        
        page2 = self.create_mount_page()
        notebook.append_page(page2, Gtk.Label(label="Monter & Utiliser"))
        
        page3 = self.create_check_page()
        notebook.append_page(page3, Gtk.Label(label="Vérifier & Réparer"))
        
        page4 = self.create_stats_page()
        notebook.append_page(page4, Gtk.Label(label="Statistiques"))
        
        # Barre de statut
        self.statusbar = Gtk.Statusbar()
        vbox.pack_start(self.statusbar, False, False, 0)
        
        self.check_installation()
        
    def find_binary(self, name):
        """Rechercher un binaire"""
        paths = [
            f"/usr/local/sbin/{name}",
            f"/usr/local/bin/{name}",
            f"/usr/sbin/{name}",
            f"/usr/bin/{name}",
        ]
        for path in paths:
            if os.path.exists(path) and os.access(path, os.X_OK):
                return path
        
        try:
            result = subprocess.run(["which", name], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
        except:
            pass
        return None
    
    def check_installation(self):
        """Vérifier l'installation"""
        missing = []
        if not self.mkfs_path:
            missing.append("mkfs.minifs")
        if not self.mount_path:
            missing.append("minifs_fuse")
        
        if missing:
            msg = f"⚠️ Binaires manquants : {', '.join(missing)}"
            self.update_status(msg)
        else:
            self.update_status("✓ Tous les binaires sont installés")
    
    def create_menubar(self):
        """Créer la barre de menu"""
        menubar = Gtk.MenuBar()
        
        file_menu = Gtk.Menu()
        file_item = Gtk.MenuItem(label="Fichier")
        file_item.set_submenu(file_menu)
        
        quit_item = Gtk.MenuItem(label="Quitter")
        quit_item.connect("activate", self.on_quit)
        file_menu.append(quit_item)
        
        menubar.append(file_item)
        
        help_menu = Gtk.Menu()
        help_item = Gtk.MenuItem(label="Aide")
        help_item.set_submenu(help_menu)
        
        about_item = Gtk.MenuItem(label="À propos")
        about_item.connect("activate", self.show_about)
        help_menu.append(about_item)
        
        menubar.append(help_item)
        
        return menubar
    
    def create_toolbar(self):
        """Créer la barre d'outils"""
        toolbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        toolbar.set_border_width(5)
        
        info_label = Gtk.Label()
        if self.mkfs_path and self.mount_path:
            info_label.set_markup(
                f"<small>mkfs: {self.mkfs_path} | mount: {self.mount_path}</small>"
            )
        else:
            info_label.set_markup("<small><b>⚠️ Installation incomplète</b></small>")
        
        toolbar.pack_start(info_label, True, True, 0)
        
        # Bouton RESET
        reset_btn = Gtk.Button(label="🔄 RESET COMPLET")
        reset_btn.connect("clicked", self.reset_everything)
        toolbar.pack_end(reset_btn, False, False, 0)
        
        return toolbar
    
    def create_format_page(self):
        """Page de création et formatage - AMÉLIORÉE"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        box.set_border_width(10)
        
        label = Gtk.Label()
        label.set_markup("<big><b>Créer et Formater une Image MiniFS</b></big>")
        box.pack_start(label, False, False, 0)
        
        # Nom de l'image
        hbox1 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox1.pack_start(Gtk.Label(label="Nom de l'image:"), False, False, 0)
        self.image_name_entry = Gtk.Entry()
        self.image_name_entry.set_text(os.path.expanduser("~/minifs.img"))
        hbox1.pack_start(self.image_name_entry, True, True, 0)
        browse_btn = Gtk.Button(label="Parcourir...")
        browse_btn.connect("clicked", self.browse_image_location)
        hbox1.pack_start(browse_btn, False, False, 0)
        box.pack_start(hbox1, False, False, 0)
        
        # Taille
        hbox2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox2.pack_start(Gtk.Label(label="Taille (MB):"), False, False, 0)
        self.size_spin = Gtk.SpinButton()
        self.size_spin.set_range(10, 10000)
        self.size_spin.set_value(100)
        self.size_spin.set_increments(10, 100)
        hbox2.pack_start(self.size_spin, True, True, 0)
        box.pack_start(hbox2, False, False, 0)
        
        # Nombre d'inodes
        hbox3 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox3.pack_start(Gtk.Label(label="Nombre d'inodes:"), False, False, 0)
        self.inodes_spin = Gtk.SpinButton()
        self.inodes_spin.set_range(512, 65536)
        self.inodes_spin.set_value(4096)
        self.inodes_spin.set_increments(512, 2048)
        hbox3.pack_start(self.inodes_spin, True, True, 0)
        box.pack_start(hbox3, False, False, 0)
        
        # NOUVEAUTÉ: Deux boutons séparés
        hbox4 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        create_only_btn = Gtk.Button(label="1️⃣ Créer l'Image Seulement")
        create_only_btn.connect("clicked", self.create_image_only)
        hbox4.pack_start(create_only_btn, True, True, 0)
        
        format_only_btn = Gtk.Button(label="2️⃣ Formater l'Image")
        format_only_btn.connect("clicked", self.format_image_only)
        hbox4.pack_start(format_only_btn, True, True, 0)
        
        box.pack_start(hbox4, False, False, 0)
        
        # Bouton combiné (comme avant)
        create_and_format_btn = Gtk.Button(label="⚡ Créer ET Formater (Rapide)")
        create_and_format_btn.connect("clicked", self.create_and_format)
        box.pack_start(create_and_format_btn, False, False, 10)
        
        # Zone de log
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_vexpand(True)
        self.format_log = Gtk.TextView()
        self.format_log.set_editable(False)
        self.format_log.set_wrap_mode(Gtk.WrapMode.WORD)
        scrolled.add(self.format_log)
        box.pack_start(scrolled, True, True, 0)
        
        return box
    
    def create_mount_page(self):
        """Page de montage - AMÉLIORÉE avec montage direct"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        box.set_border_width(10)
        
        label = Gtk.Label()
        label.set_markup("<big><b>Monter/Démonter une Image MiniFS</b></big>")
        box.pack_start(label, False, False, 0)
        
        # Image
        hbox1 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox1.pack_start(Gtk.Label(label="Image:"), False, False, 0)
        self.mount_image_entry = Gtk.Entry()
        self.mount_image_entry.set_text(os.path.expanduser("~/minifs.img"))
        hbox1.pack_start(self.mount_image_entry, True, True, 0)
        browse_mount_btn = Gtk.Button(label="Parcourir...")
        browse_mount_btn.connect("clicked", self.browse_mount_image)
        hbox1.pack_start(browse_mount_btn, False, False, 0)
        box.pack_start(hbox1, False, False, 0)
        
        # Point de montage
        hbox2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox2.pack_start(Gtk.Label(label="Point de montage:"), False, False, 0)
        self.mountpoint_entry = Gtk.Entry()
        self.mountpoint_entry.set_text(os.path.expanduser("~/mnt-minifs"))
        hbox2.pack_start(self.mountpoint_entry, True, True, 0)
        box.pack_start(hbox2, False, False, 0)
        
        # NOUVEAUTÉ: Bouton de montage direct avec pkexec
        hbox3 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        
        self.mount_btn = Gtk.Button(label="🚀 Monter (Direct)")
        self.mount_btn.connect("clicked", self.mount_fs_direct)
        hbox3.pack_start(self.mount_btn, True, True, 0)
        
        self.unmount_btn = Gtk.Button(label="⏹️ Démonter")
        self.unmount_btn.connect("clicked", self.unmount_fs)
        hbox3.pack_start(self.unmount_btn, True, True, 0)
        
        box.pack_start(hbox3, False, False, 10)
        
        # Statut de montage
        self.mount_status_label = Gtk.Label()
        self.mount_status_label.set_markup("<i>Non monté</i>")
        box.pack_start(self.mount_status_label, False, False, 0)
        
        # Explorateur de fichiers
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_vexpand(True)
        self.file_list = Gtk.TextView()
        self.file_list.set_editable(False)
        font = Pango.FontDescription("monospace 10")
        self.file_list.modify_font(font)
        scrolled.add(self.file_list)
        box.pack_start(scrolled, True, True, 0)
        
        # Bouton rafraîchir
        refresh_btn = Gtk.Button(label="🔄 Rafraîchir la Liste")
        refresh_btn.connect("clicked", self.refresh_file_list)
        box.pack_start(refresh_btn, False, False, 0)
        
        return box
    
    def create_check_page(self):
        """Page de vérification"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        box.set_border_width(10)
        
        label = Gtk.Label()
        label.set_markup("<big><b>Vérifier et Réparer</b></big>")
        box.pack_start(label, False, False, 0)
        
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox.pack_start(Gtk.Label(label="Image:"), False, False, 0)
        self.check_image_entry = Gtk.Entry()
        hbox.pack_start(self.check_image_entry, True, True, 0)
        browse_check_btn = Gtk.Button(label="Parcourir...")
        browse_check_btn.connect("clicked", self.browse_check_image)
        hbox.pack_start(browse_check_btn, False, False, 0)
        box.pack_start(hbox, False, False, 0)
        
        check_btn = Gtk.Button(label="Vérifier le Système de Fichiers")
        check_btn.connect("clicked", self.check_fs)
        box.pack_start(check_btn, False, False, 10)
        
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_vexpand(True)
        self.check_log = Gtk.TextView()
        self.check_log.set_editable(False)
        scrolled.add(self.check_log)
        box.pack_start(scrolled, True, True, 0)
        
        return box
    
    def create_stats_page(self):
        """Page de statistiques"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        box.set_border_width(10)
        
        label = Gtk.Label()
        label.set_markup("<big><b>Statistiques du Système de Fichiers</b></big>")
        box.pack_start(label, False, False, 0)
        
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox.pack_start(Gtk.Label(label="Image:"), False, False, 0)
        self.stats_image_entry = Gtk.Entry()
        hbox.pack_start(self.stats_image_entry, True, True, 0)
        browse_stats_btn = Gtk.Button(label="Parcourir...")
        browse_stats_btn.connect("clicked", self.browse_stats_image)
        hbox.pack_start(browse_stats_btn, False, False, 0)
        box.pack_start(hbox, False, False, 0)
        
        hbox2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        stats_btn = Gtk.Button(label="📊 Afficher les Statistiques")
        stats_btn.connect("clicked", self.show_stats)
        hbox2.pack_start(stats_btn, True, True, 0)
        
        frag_btn = Gtk.Button(label="📈 Analyser la Fragmentation")
        frag_btn.connect("clicked", self.show_fragmentation)
        hbox2.pack_start(frag_btn, True, True, 0)
        box.pack_start(hbox2, False, False, 10)
        
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_vexpand(True)
        self.stats_log = Gtk.TextView()
        self.stats_log.set_editable(False)
        font = Pango.FontDescription("monospace 10")
        self.stats_log.modify_font(font)
        scrolled.add(self.stats_log)
        box.pack_start(scrolled, True, True, 0)
        
        return box
    
    def create_image_only(self, widget):
        """NOUVEAU: Créer l'image seulement (sans formater)"""
        image_path = self.image_name_entry.get_text()
        size_mb = int(self.size_spin.get_value())
        
        def worker():
            try:
                GLib.idle_add(self.append_to_textview, self.format_log,
                             f"Création de {image_path} ({size_mb} MB)...")
                
                subprocess.run([
                    "dd", "if=/dev/zero", f"of={image_path}",
                    "bs=1M", f"count={size_mb}"
                ], capture_output=True, check=True)
                
                GLib.idle_add(self.append_to_textview, self.format_log,
                             "✓ Image créée avec succès!")
                GLib.idle_add(self.append_to_textview, self.format_log,
                             "")
                GLib.idle_add(self.append_to_textview, self.format_log,
                             "Vous pouvez maintenant cliquer sur '2️⃣ Formater l'Image'")
                GLib.idle_add(self.append_to_textview, self.format_log,
                             "OU utiliser l'image telle quelle")
                GLib.idle_add(self.update_status, "Image créée (non formatée)")
                
            except Exception as e:
                GLib.idle_add(self.append_to_textview, self.format_log,
                             f"✗ Erreur: {str(e)}")
        
        thread = threading.Thread(target=worker)
        thread.daemon = True
        thread.start()
    
    def format_image_only(self, widget):
        """NOUVEAU: Formater une image existante"""
        if not self.mkfs_path:
            self.show_error("mkfs.minifs non trouvé !")
            return
        
        image_path = self.image_name_entry.get_text()
        num_inodes = int(self.inodes_spin.get_value())
        
        if not os.path.exists(image_path):
            self.show_error(f"L'image {image_path} n'existe pas!\nCréez-la d'abord avec '1️⃣ Créer l'Image'")
            return
        
        def worker():
            try:
                GLib.idle_add(self.append_to_textview, self.format_log,
                             f"Formatage de {image_path} avec {num_inodes} inodes...")
                
                result = subprocess.run([
                    self.mkfs_path, image_path, "--inodes", str(num_inodes), "--force"
                ], capture_output=True, text=True)
                
                GLib.idle_add(self.append_to_textview, self.format_log, result.stdout)
                
                if result.returncode == 0:
                    GLib.idle_add(self.append_to_textview, self.format_log,
                                 "✓ Formatage réussi!")
                    GLib.idle_add(self.update_status, "Image formatée")
                    GLib.idle_add(self.mount_image_entry.set_text, image_path)
                else:
                    GLib.idle_add(self.append_to_textview, self.format_log,
                                 f"✗ Erreur: {result.stderr}")
                    
            except Exception as e:
                GLib.idle_add(self.append_to_textview, self.format_log,
                             f"✗ Erreur: {str(e)}")
        
        thread = threading.Thread(target=worker)
        thread.daemon = True
        thread.start()
    
    def create_and_format(self, widget):
        """Créer ET formater (comme avant)"""
        if not self.mkfs_path:
            self.show_error("mkfs.minifs non trouvé !")
            return
        
        image_path = self.image_name_entry.get_text()
        size_mb = int(self.size_spin.get_value())
        num_inodes = int(self.inodes_spin.get_value())
        
        def worker():
            try:
                GLib.idle_add(self.append_to_textview, self.format_log,
                             f"Création de {image_path} ({size_mb} MB)...")
                
                subprocess.run([
                    "dd", "if=/dev/zero", f"of={image_path}",
                    "bs=1M", f"count={size_mb}"
                ], capture_output=True, check=True)
                
                GLib.idle_add(self.append_to_textview, self.format_log, "✓ Image créée")
                GLib.idle_add(self.append_to_textview, self.format_log,
                             f"Formatage avec {num_inodes} inodes...")
                
                result = subprocess.run([
                    self.mkfs_path, image_path, "--inodes", str(num_inodes), "--force"
                ], capture_output=True, text=True)
                
                GLib.idle_add(self.append_to_textview, self.format_log, result.stdout)
                
                if result.returncode == 0:
                    GLib.idle_add(self.append_to_textview, self.format_log,
                                 "✓ Création et formatage réussis!")
                    GLib.idle_add(self.update_status, "Image créée et formatée")
                    GLib.idle_add(self.mount_image_entry.set_text, image_path)
                else:
                    GLib.idle_add(self.append_to_textview, self.format_log,
                                 f"✗ Erreur: {result.stderr}")
                    
            except Exception as e:
                GLib.idle_add(self.append_to_textview, self.format_log,
                             f"✗ Erreur: {str(e)}")
        
        thread = threading.Thread(target=worker)
        thread.daemon = True
        thread.start()
    
    def mount_fs_direct(self, widget):
        """NOUVEAU: Montage direct avec pkexec (demande mot de passe graphique)"""
        if not self.mount_path:
            self.show_error("minifs_fuse non trouvé !")
            return
        
        image = self.mount_image_entry.get_text()
        mountpoint = self.mountpoint_entry.get_text()
        
        if not os.path.exists(image):
            self.show_error(f"Image {image} introuvable!")
            return
        
        # Créer le point de montage
        try:
            os.makedirs(mountpoint, exist_ok=True)
        except:
            pass
        
        def worker():
            try:
                GLib.idle_add(self.update_status, "Montage en cours...")
                
                # Utiliser pkexec pour demander les droits root avec interface graphique
                result = subprocess.run([
                    "pkexec", self.mount_path, image, mountpoint,
                    "-o", "default_permissions,allow_other,umask=000"
                ], capture_output=True, text=True)
                
                if result.returncode == 0:
                    time.sleep(1)
                    # Vérifier que c'est monté
                    mount_check = subprocess.run(
                        ["mount"], capture_output=True, text=True
                    )
                    
                    if mountpoint in mount_check.stdout:
                        self.current_image = image
                        self.current_mountpoint = mountpoint
                        self.is_mounted = True
                        
                        GLib.idle_add(self.mount_status_label.set_markup,
                                     f"<b>✓ Monté: {image} sur {mountpoint}</b>")
                        GLib.idle_add(self.mount_btn.set_sensitive, False)
                        GLib.idle_add(self.unmount_btn.set_sensitive, True)
                        GLib.idle_add(self.update_status, f"✓ Monté sur {mountpoint}")
                        GLib.idle_add(self.refresh_file_list, None)
                    else:
                        GLib.idle_add(self.show_error, "Le montage a échoué")
                else:
                    error_msg = result.stderr or "Erreur inconnue"
                    GLib.idle_add(self.show_error, f"Erreur de montage:\n{error_msg}")
                    
            except Exception as e:
                GLib.idle_add(self.show_error, f"Erreur: {str(e)}")
        
        thread = threading.Thread(target=worker)
        thread.daemon = True
        thread.start()
    
    def unmount_fs(self, widget):
        """Démonter"""
        mountpoint = self.mountpoint_entry.get_text()
        
        if not mountpoint:
            self.show_error("Spécifiez un point de montage")
            return
        
        def worker():
            try:
                # Essayer fusermount3 puis umount
                result1 = subprocess.run(
                    ["pkexec", "fusermount3", "-u", mountpoint],
                    capture_output=True, text=True
                )
                
                result2 = subprocess.run(
                    ["pkexec", "umount", mountpoint],
                    capture_output=True, text=True
                )
                
                if result1.returncode == 0 or result2.returncode == 0:
                    self.is_mounted = False
                    GLib.idle_add(self.mount_status_label.set_markup, "<i>Non monté</i>")
                    GLib.idle_add(self.mount_btn.set_sensitive, True)
                    GLib.idle_add(self.unmount_btn.set_sensitive, False)
                    GLib.idle_add(self.update_status, "Démonté")
                    GLib.idle_add(self.file_list.get_buffer().set_text, "")
                else:
                    GLib.idle_add(self.show_warning, "Démontage",
                                 f"Impossible de démonter {mountpoint}")
            except Exception as e:
                GLib.idle_add(self.show_error, f"Erreur: {str(e)}")
        
        thread = threading.Thread(target=worker)
        thread.daemon = True
        thread.start()
    
    def refresh_file_list(self, widget):
        """Rafraîchir la liste"""
        mountpoint = self.mountpoint_entry.get_text()
        
        if not os.path.exists(mountpoint):
            self.file_list.get_buffer().set_text(f"{mountpoint} n'existe pas")
            return
        
        try:
            result = subprocess.run(
                ["ls", "-lah", mountpoint],
                capture_output=True, text=True
            )
            self.file_list.get_buffer().set_text(result.stdout)
        except Exception as e:
            self.file_list.get_buffer().set_text(f"Erreur: {str(e)}")
    
    def check_fs(self, widget):
        """Vérifier"""
        if not self.fsck_path:
            self.show_error("fsck.minifs non trouvé !")
            return
        
        image = self.check_image_entry.get_text()
        
        if not os.path.exists(image):
            self.show_error(f"Image {image} introuvable!")
            return
        
        try:
            result = subprocess.run(
                [self.fsck_path, image],
                capture_output=True, text=True
            )
            
            buffer = self.check_log.get_buffer()
            buffer.set_text(result.stdout + "\n" + result.stderr)
            
            if result.returncode == 0:
                self.update_status("Vérification OK")
            else:
                self.update_status("Erreurs détectées")
                
        except Exception as e:
            self.show_error(f"Erreur: {str(e)}")
    
    def show_stats(self, widget):
        """Statistiques"""
        if not self.debugfs_path:
            self.show_error("debugfs.minifs non trouvé !")
            return
        
        image = self.stats_image_entry.get_text()
        
        if not os.path.exists(image):
            self.show_error(f"Image {image} introuvable!")
            return
        
        try:
            result = subprocess.run(
                [self.debugfs_path, image, "--stats"],
                capture_output=True, text=True
            )
            
            buffer = self.stats_log.get_buffer()
            buffer.set_text(result.stdout)
            
        except Exception as e:
            self.show_error(f"Erreur: {str(e)}")
    
    def show_fragmentation(self, widget):
        """Fragmentation"""
        if not self.debugfs_path:
            self.show_error("debugfs.minifs non trouvé !")
            return
        
        image = self.stats_image_entry.get_text()
        
        if not os.path.exists(image):
            self.show_error(f"Image {image} introuvable!")
            return
        
        try:
            result = subprocess.run(
                [self.debugfs_path, image, "--fragmentation"],
                capture_output=True, text=True
            )
            
            buffer = self.stats_log.get_buffer()
            buffer.set_text(result.stdout)
            
        except Exception as e:
            self.show_error(f"Erreur: {str(e)}")
    
    def reset_everything(self, widget):
        """RESET complet"""
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.WARNING,
            buttons=Gtk.ButtonsType.YES_NO,
            text="RESET COMPLET",
        )
        dialog.format_secondary_text(
            "Démonter tout et réinitialiser l'interface ?"
        )
        
        response = dialog.run()
        dialog.destroy()
        
        if response == Gtk.ResponseType.YES:
            subprocess.run(["pkexec", "pkill", "-9", "minifs_fuse"], check=False)
            
            # Réinitialiser
            self.is_mounted = False
            self.mount_status_label.set_markup("<i>Non monté</i>")
            self.mount_btn.set_sensitive(True)
            self.unmount_btn.set_sensitive(False)
            
            for tv in [self.format_log, self.file_list, self.check_log, self.stats_log]:
                tv.get_buffer().set_text("")
            
            self.update_status("Reset effectué")
    
    def update_status(self, message):
        """Mettre à jour la barre de statut"""
        self.statusbar.pop(0)
        self.statusbar.push(0, message)
    
    def append_to_textview(self, textview, text):
        """Ajouter du texte"""
        buffer = textview.get_buffer()
        end_iter = buffer.get_end_iter()
        buffer.insert(end_iter, text + "\n")
    
    def browse_image_location(self, widget):
        self._browse_save_file(self.image_name_entry)
    
    def browse_mount_image(self, widget):
        self._browse_open_file(self.mount_image_entry)
    
    def browse_check_image(self, widget):
        self._browse_open_file(self.check_image_entry)
    
    def browse_stats_image(self, widget):
        self._browse_open_file(self.stats_image_entry)
    
    def _browse_save_file(self, entry):
        dialog = Gtk.FileChooserDialog(
            title="Choisir l'emplacement",
            parent=self,
            action=Gtk.FileChooserAction.SAVE
        )
        dialog.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_SAVE, Gtk.ResponseType.OK
        )
        
        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            entry.set_text(dialog.get_filename())
        dialog.destroy()
    
    def _browse_open_file(self, entry):
        dialog = Gtk.FileChooserDialog(
            title="Choisir une image",
            parent=self,
            action=Gtk.FileChooserAction.OPEN
        )
        dialog.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OPEN, Gtk.ResponseType.OK
        )
        
        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            entry.set_text(dialog.get_filename())
        dialog.destroy()
    
    def show_about(self, widget):
        dialog = Gtk.AboutDialog()
        dialog.set_transient_for(self)
        dialog.set_program_name("MiniFS GUI")
        dialog.set_version("2.0.0")
        dialog.set_comments("Interface graphique améliorée pour MiniFS\nAvec montage direct et options séparées")
        dialog.run()
        dialog.destroy()
    
    def show_error(self, message):
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.ERROR,
            buttons=Gtk.ButtonsType.OK,
            text=message,
        )
        dialog.run()
        dialog.destroy()
    
    def show_warning(self, title, message):
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.WARNING,
            buttons=Gtk.ButtonsType.OK,
            text=title,
        )
        dialog.format_secondary_text(message)
        dialog.run()
        dialog.destroy()
    
    def on_quit(self, widget):
        if self.mount_process:
            try:
                self.mount_process.terminate()
            except:
                pass
        Gtk.main_quit()

if __name__ == "__main__":
    win = MinifsGUI()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    Gtk.main()

#!/usr/bin/env python3
"""
MiniFS GUI - Interface Graphique Améliorée avec Reset
"""

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib, Pango
import subprocess
import os
import threading
import shutil

class MinifsGUI(Gtk.Window):
    def __init__(self):
        super().__init__(title="MiniFS - Gestionnaire de Système de Fichiers")
        self.set_border_width(10)
        self.set_default_size(900, 700)
        
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
        
        # Barre d'outils avec RESET
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
        
        # Vérifier les binaires au démarrage
        self.check_installation()
        
    def find_binary(self, name):
        """Rechercher un binaire avec plus de chemins"""
        paths = [
            f"/usr/local/sbin/{name}",
            f"/usr/local/bin/{name}",
            f"/usr/sbin/{name}",
            f"/usr/bin/{name}",
            f"{os.path.expanduser('~')}/minifs-fuse/bin/{name}",
        ]
        for path in paths:
            if os.path.exists(path) and os.access(path, os.X_OK):
                return path
        
        # Chercher dans PATH
        try:
            result = subprocess.run(["which", name], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
        except:
            pass
        return None
    
    def check_installation(self):
        """Vérifier l'installation au démarrage"""
        missing = []
        if not self.mkfs_path:
            missing.append("mkfs.minifs")
        if not self.mount_path:
            missing.append("minifs_fuse")
        if not self.fsck_path:
            missing.append("fsck.minifs")
        
        if missing:
            msg = f"⚠️ Binaires manquants : {', '.join(missing)}\n"
            msg += "Installez MiniFS avec: sudo make install"
            self.update_status(msg)
            self.show_warning("Installation incomplète", msg)
        else:
            self.update_status("✓ Tous les binaires sont installés")
    
    def create_menubar(self):
        """Créer la barre de menu"""
        menubar = Gtk.MenuBar()
        
        # Menu Fichier
        file_menu = Gtk.Menu()
        file_item = Gtk.MenuItem(label="Fichier")
        file_item.set_submenu(file_menu)
        
        quit_item = Gtk.MenuItem(label="Quitter")
        quit_item.connect("activate", self.on_quit)
        file_menu.append(quit_item)
        
        menubar.append(file_item)
        
        # Menu Aide
        help_menu = Gtk.Menu()
        help_item = Gtk.MenuItem(label="Aide")
        help_item.set_submenu(help_menu)
        
        about_item = Gtk.MenuItem(label="À propos")
        about_item.connect("activate", self.show_about)
        help_menu.append(about_item)
        
        menubar.append(help_item)
        
        return menubar
    
    def create_toolbar(self):
        """Créer la barre d'outils avec bouton RESET"""
        toolbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        toolbar.set_border_width(5)
        
        # Informations sur les binaires
        info_label = Gtk.Label()
        paths_info = []
        if self.mkfs_path:
            paths_info.append(f"mkfs: {self.mkfs_path}")
        if self.mount_path:
            paths_info.append(f"mount: {self.mount_path}")
        
        if paths_info:
            info_label.set_markup(f"<small>{' | '.join(paths_info)}</small>")
        else:
            info_label.set_markup("<small><b>⚠️ Binaires non trouvés</b></small>")
        
        toolbar.pack_start(info_label, True, True, 0)
        
        # Bouton RESET (en rouge)
        reset_btn = Gtk.Button(label="🔄 RESET COMPLET")
        reset_btn.connect("clicked", self.reset_everything)
        
        # Style rouge pour le bouton
        style_provider = Gtk.CssProvider()
        css = b"""
        button {
            background: #e74c3c;
            color: white;
            font-weight: bold;
            padding: 10px;
        }
        button:hover {
            background: #c0392b;
        }
        """
        style_provider.load_from_data(css)
        context = reset_btn.get_style_context()
        context.add_provider(style_provider, Gtk.STYLE_PROVIDER_PRIORITY_USER)
        
        toolbar.pack_end(reset_btn, False, False, 0)
        
        return toolbar
    
    def create_format_page(self):
        """Page de création et formatage"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        box.set_border_width(10)
        
        label = Gtk.Label()
        label.set_markup("<big><b>Créer une nouvelle image MiniFS</b></big>")
        box.pack_start(label, False, False, 0)
        
        # Nom du fichier
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
        
        # Bouton créer
        create_btn = Gtk.Button(label="Créer et Formater")
        create_btn.connect("clicked", self.create_and_format)
        box.pack_start(create_btn, False, False, 10)
        
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
        """Page de montage avec corrections"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        box.set_border_width(10)
        
        label = Gtk.Label()
        label.set_markup("<big><b>Monter/Démonter une image MiniFS</b></big>")
        box.pack_start(label, False, False, 0)
        
        # Image à monter
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
        
        # Avertissement
        warning_label = Gtk.Label()
        warning_label.set_markup(
            "<small><b>Note:</b> Le montage nécessite sudo. "
            "Utilisez la ligne de commande pour monter:\n"
            f"<tt>sudo {self.mount_path or 'minifs_fuse'} IMAGE MOUNTPOINT -o default_permissions</tt></small>"
        )
        warning_label.set_line_wrap(True)
        box.pack_start(warning_label, False, False, 0)
        
        # Boutons
        hbox3 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.mount_btn = Gtk.Button(label="Générer la commande")
        self.mount_btn.connect("clicked", self.generate_mount_command)
        hbox3.pack_start(self.mount_btn, True, True, 0)
        
        self.unmount_btn = Gtk.Button(label="Démonter (sudo)")
        self.unmount_btn.connect("clicked", self.unmount_fs)
        hbox3.pack_start(self.unmount_btn, True, True, 0)
        
        box.pack_start(hbox3, False, False, 10)
        
        # Zone de commandes
        scrolled_cmd = Gtk.ScrolledWindow()
        scrolled_cmd.set_min_content_height(100)
        self.mount_commands = Gtk.TextView()
        self.mount_commands.set_editable(False)
        self.mount_commands.set_wrap_mode(Gtk.WrapMode.WORD)
        scrolled_cmd.add(self.mount_commands)
        box.pack_start(scrolled_cmd, False, False, 0)
        
        # Explorateur de fichiers
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_vexpand(True)
        self.file_list = Gtk.TextView()
        self.file_list.set_editable(False)
        scrolled.add(self.file_list)
        box.pack_start(scrolled, True, True, 0)
        
        # Bouton rafraîchir
        refresh_btn = Gtk.Button(label="Rafraîchir la liste")
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
        
        check_btn = Gtk.Button(label="Vérifier le système de fichiers")
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
        stats_btn = Gtk.Button(label="Afficher les statistiques")
        stats_btn.connect("clicked", self.show_stats)
        hbox2.pack_start(stats_btn, True, True, 0)
        
        frag_btn = Gtk.Button(label="Analyser la fragmentation")
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
    
    def reset_everything(self, widget):
        """RESET COMPLET - Nettoyer tout"""
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.WARNING,
            buttons=Gtk.ButtonsType.YES_NO,
            text="RESET COMPLET",
        )
        dialog.format_secondary_text(
            "Cela va:\n"
            "• Démonter tous les systèmes MiniFS montés\n"
            "• Tuer tous les processus minifs_fuse\n"
            "• Nettoyer les points de montage\n"
            "• Réinitialiser l'interface\n\n"
            "Continuer ?"
        )
        
        response = dialog.run()
        dialog.destroy()
        
        if response == Gtk.ResponseType.YES:
            self.perform_reset()
    
    def perform_reset(self):
        """Effectuer le reset"""
        reset_log = []
        
        # 1. Démonter tous les montages MiniFS
        try:
            result = subprocess.run(
                ["mount"], capture_output=True, text=True
            )
            for line in result.stdout.split('\n'):
                if 'minifs' in line.lower() or 'fuse' in line:
                    parts = line.split()
                    if len(parts) >= 3:
                        mountpoint = parts[2]
                        try:
                            subprocess.run(["fusermount3", "-u", mountpoint], check=False)
                            subprocess.run(["sudo", "umount", mountpoint], check=False)
                            reset_log.append(f"✓ Démonté: {mountpoint}")
                        except:
                            pass
        except:
            pass
        
        # 2. Tuer tous les processus minifs_fuse
        try:
            subprocess.run(["pkill", "-9", "minifs_fuse"], check=False)
            reset_log.append("✓ Processus minifs_fuse terminés")
        except:
            pass
        
        # 3. Réinitialiser les variables
        self.current_image = None
        self.current_mountpoint = None
        self.mount_process = None
        self.is_mounted = False
        
        # 4. Nettoyer les TextViews
        for textview in [self.format_log, self.file_list, self.check_log, 
                         self.stats_log, self.mount_commands]:
            textview.get_buffer().set_text("")
        
        # 5. Réactiver les boutons
        self.mount_btn.set_sensitive(True)
        self.unmount_btn.set_sensitive(True)
        
        reset_log.append("✓ Interface réinitialisée")
        
        # Afficher le résultat
        message = "\n".join(reset_log)
        self.show_info("Reset terminé", message)
        self.update_status("Reset effectué")
    
    def generate_mount_command(self, widget):
        """Générer la commande de montage"""
        image = self.mount_image_entry.get_text()
        mountpoint = self.mountpoint_entry.get_text()
        
        if not self.mount_path:
            self.show_error("minifs_fuse non trouvé !")
            return
        
        commands = f"""# Commandes à exécuter dans le terminal:

# 1. Créer le point de montage
mkdir -p {mountpoint}

# 2. Monter (en arrière-plan)
sudo {self.mount_path} {image} {mountpoint} -o default_permissions &

# 3. Vérifier
ls -la {mountpoint}

# 4. Utiliser normalement
echo "Test" > {mountpoint}/test.txt
cat {mountpoint}/test.txt

# 5. Démonter quand terminé
sudo fusermount3 -u {mountpoint}
"""
        
        buffer = self.mount_commands.get_buffer()
        buffer.set_text(commands)
        
        self.update_status("Commandes générées - Copiez-les dans un terminal")
    
    def unmount_fs(self, widget):
        """Démonter avec sudo"""
        mountpoint = self.mountpoint_entry.get_text()
        
        if not mountpoint:
            self.show_error("Spécifiez un point de montage")
            return
        
        try:
            # Essayer fusermount3
            result1 = subprocess.run(
                ["sudo", "fusermount3", "-u", mountpoint],
                capture_output=True, text=True
            )
            
            # Essayer umount
            result2 = subprocess.run(
                ["sudo", "umount", mountpoint],
                capture_output=True, text=True
            )
            
            if result1.returncode == 0 or result2.returncode == 0:
                self.show_info("Succès", f"Démonté: {mountpoint}")
                self.update_status("Démonté")
                self.refresh_file_list(None)
            else:
                self.show_warning("Démontage", 
                    f"Impossible de démonter {mountpoint}\n"
                    f"Erreur: {result1.stderr or result2.stderr}")
        except Exception as e:
            self.show_error(f"Erreur: {str(e)}")
    
    def refresh_file_list(self, widget):
        """Rafraîchir la liste des fichiers"""
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
    
    def create_and_format(self, widget):
        """Créer et formater"""
        if not self.mkfs_path:
            self.show_error("mkfs.minifs non trouvé !\nInstallez avec: sudo make install")
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
                    GLib.idle_add(self.append_to_textview, self.format_log, "✓ Formatage réussi!")
                    GLib.idle_add(self.update_status, "Image créée et formatée")
                    GLib.idle_add(self.mount_image_entry.set_text, image_path)
                else:
                    GLib.idle_add(self.append_to_textview, self.format_log, f"✗ Erreur: {result.stderr}")
                    
            except Exception as e:
                GLib.idle_add(self.append_to_textview, self.format_log, f"✗ Erreur: {str(e)}")
        
        thread = threading.Thread(target=worker)
        thread.daemon = True
        thread.start()
    
    def check_fs(self, widget):
        """Vérifier le système de fichiers"""
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
        """Afficher les statistiques"""
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
        """Afficher la fragmentation"""
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
        """Parcourir pour sauvegarder"""
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
        """Parcourir un fichier"""
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
        """À propos"""
        dialog = Gtk.AboutDialog()
        dialog.set_transient_for(self)
        dialog.set_program_name("MiniFS GUI")
        dialog.set_version("1.0.1")
        dialog.set_comments("Interface graphique pour MiniFS\nAvec fonction RESET")
        dialog.run()
        dialog.destroy()
    
    def show_error(self, message):
        """Afficher une erreur"""
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
        """Afficher un avertissement"""
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
    
    def show_info(self, title, message):
        """Afficher une info"""
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.INFO,
            buttons=Gtk.ButtonsType.OK,
            text=title,
        )
        dialog.format_secondary_text(message)
        dialog.run()
        dialog.destroy()
    
    def on_quit(self, widget):
        """Quitter proprement"""
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

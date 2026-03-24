#!/bin/bash
# MiniFS Uninstall Script

echo "Uninstalling MiniFS..."

sudo rm -f /usr/local/bin/minifs_fuse
sudo rm -f /usr/local/sbin/mkfs.minifs
sudo rm -f /usr/local/sbin/fsck.minifs
sudo rm -f /usr/local/sbin/debugfs.minifs
sudo rm -f /usr/local/sbin/mount.minifs

echo "MiniFS uninstalled successfully"

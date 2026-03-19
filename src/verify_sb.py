#!/usr/bin/env python3
"""verify_sb.py — Vérifie le superbloc d'une image MiniFS."""
import struct, sys

MINIFS_MAGIC = 0x4D494E49
BLOCK_SIZE   = 4096

if len(sys.argv) < 2:
    print("Usage: verify_sb.py <image>")
    sys.exit(1)

with open(sys.argv[1], 'rb') as f:
    data = f.read(BLOCK_SIZE)

magic, bsize, total, total_i, free_b, free_i, it_start, d_start = \
    struct.unpack_from('<8I', data)

print("--- Vérification du Superbloc ---")
print(f"  magic        = 0x{magic:08X}  (attendu 0x{MINIFS_MAGIC:08X})")
print(f"  block_size   = {bsize}")
print(f"  total_blocs  = {total}")
print(f"  total_inodes = {total_i}")
print(f"  free_blocs   = {free_b}")
print(f"  free_inodes  = {free_i}  (attendu 127)")
print(f"  inode_start  = {it_start}  (attendu 3)")
print(f"  data_start   = {d_start}  (attendu 5)")

assert magic    == MINIFS_MAGIC, f"MAGIC INVALIDE : 0x{magic:08X}"
assert bsize    == BLOCK_SIZE,   f"block_size incorrect : {bsize}"
assert total_i  == 128,          f"total_inodes incorrect : {total_i}"
assert free_i   == 127,          f"free_inodes incorrect : {free_i}"
assert it_start == 3,            f"inode_table_start incorrect : {it_start}"
assert d_start  == 5,            f"data_start incorrect : {d_start}"

print("\n  ✓ Superbloc valide !")
print("--- Bitmap des inodes (octet 0) ---")

with open(sys.argv[1], 'rb') as f:
    f.seek(BLOCK_SIZE)  # bloc 1 = bitmap inodes
    bm = f.read(1)[0]
print(f"  bitmap[0] = 0b{bm:08b}  (bit 0 = 1 → inode 0 occupé ✓)")

print("--- Inode 0 (racine) ---")
with open(sys.argv[1], 'rb') as f:
    f.seek(3 * BLOCK_SIZE)  # bloc 3 = début table inodes
    inode_data = f.read(64)

itype, _res, perm, size, blocks, ctime, mtime, atime = \
    struct.unpack_from('<BBHIIiii', inode_data)
direct0 = struct.unpack_from('<I', inode_data, 24)[0]  # offset 24 = après BBHIIiii
print(f"  type        = {itype}  (2 = DIR ✓)")
print(f"  permissions = {oct(perm)}")
print(f"  size        = {size}")
print(f"  blocks_used = {blocks}")
print(f"  direct[0]   = {direct0}  (attendu 5)")

assert itype   == 2, f"type inode racine incorrect : {itype}"
assert direct0 == 5, f"direct[0] inode racine incorrect : {direct0}"
print("\n  ✓ Inode racine valide !")

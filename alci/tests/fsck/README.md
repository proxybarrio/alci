# fsck tests on large MINIX v1 filesystems

Written for ghaerr/elks issue #2851 and PR #2853.

- `mkinodes.sh <img> [nfiles] [blocks] [filler_kb]`: MINIX v1 image with many
  in-use inodes. A filler of random data goes in first (zeros would be
  stored as holes), then the small files, so their data lands at high zone
  numbers. Needs `mfs` from `elks/tools/bin`.
- `corrupt.py <img> <inode>`: clears the zone-map bit of the inode's first
  zone and sets its link count to 5.

Run with `alci/tests/qtest.py`, the image as second disk (`EXTRA_HD`,
`NOSNAP=1` to keep the repair), then check on the host with
`fsck.minix -f` (util-linux).

Results, 2026-11-16, QEMU 486, 64 MB filesystems (65535 blocks):

| Image | ELKS 22a4c5d | PR #2853 (2c2bf6b) |
| --- | --- | --- |
| 900 files (912 inodes), data up to zone 41041, `fsck -f` | clean | clean |
| 3000 files (3033 inodes), data up to zone 43249, `fsck -f` | "inode count too big" flood | clean |
| 900 files, inode 912 corrupted (zone 41041 unmarked, nlinks 5), `fsck -fa` | repaired; host fsck clean | repaired; host fsck clean |
| 3000 files, inode 3033 corrupted (zone 43249 unmarked, nlinks 5), `fsck -fa` | stack overflow, segfault; host fsck finds 31 problems | repaired; host fsck clean |

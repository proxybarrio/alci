#!/bin/bash
# mkdisk2.sh <fd720 image> <fd1440 image> <output>
# Builds disk 2 of the 720K set: every file of the 1.44M image that is not
# on the 720K image (programs already compressed as on the 1.44M image),
# plus alci/disk2/{README,setup,install-hd}. Symbolic links are skipped
# (setup and install-hd recreate them). Fails if the files do not fit.
set -e
D1=$1; D2=$2; OUT=$3
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
MFS=$ROOT/elks/tools/bin/mfs
TMP=$(mktemp -d)
trap 'rm -rf $TMP' EXIT
python3 - "$MFS" "$D1" "$D2" > $TMP/list <<'EOF'
import subprocess, sys
mfs, a_img, b_img = sys.argv[1:4]
def walk(img, d='/', acc=None):
    acc = {} if acc is None else acc
    out = subprocess.run([mfs, img, 'ls', '-l', d], capture_output=True, text=True).stdout
    for l in out.splitlines():
        p = l.split()
        if len(p) < 6 or p[-1] in ('.', '..') or l.endswith(':'):
            continue
        if p[0].startswith('l'):
            continue
        name = p[-1]; path = d.rstrip('/') + '/' + name
        if p[0].startswith('d'):
            if path != '/dev':
                walk(img, path, acc)
        elif p[0].startswith('-'):
            acc[path] = 1
    return acc
a = walk(a_img); b = walk(b_img)
for k in sorted(b):
    if k not in a:
        print(k)
EOF
while read f; do
  mkdir -p "$TMP/fs$(dirname $f)"
  $MFS "$D2" extract "$f" "$TMP/fs$f"
done < $TMP/list
chmod 755 $TMP/fs/bin/*
cp $ROOT/alci/disk2/README $ROOT/alci/disk2/setup $ROOT/alci/disk2/install-hd $TMP/fs/
rm -f "$OUT"
$MFS "$OUT" genfs -1 -n14 -i128 -s720 $TMP/fs > /dev/null
$MFS "$OUT" stat
echo "disk 2: $(wc -l < $TMP/list) files"

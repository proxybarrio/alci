#!/usr/bin/env python3
"""Strict checker for alci/tests/functional.txt output (qtest.py / t86.py).

usage: check.py <suite> <output.txt> <image> [mfs]

Every one of the suite's commands must appear, in order, with output equal
to the expected value computed independently (from the image itself where
the value depends on the image). TIMEOUT, LINE TOO LONG, BOOT TIMEOUT, a
missing command or a wrong/incomplete output are FAIL, whatever the exit
code of the harness was. Prints one line per command and a summary; exits
1 on any FAIL.
"""
import sys, os, re, subprocess, hashlib

suite, outf, img = sys.argv[1:4]
MFS = sys.argv[4] if len(sys.argv) > 4 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../elks/tools/bin/mfs')
cmds = [l for l in open(suite).read().split('\n') if l.strip() and not l.startswith('##')]

def mfs(*a):
    return subprocess.run([MFS, img] + list(a), capture_output=True).stdout
binlist = [x for x in mfs('ls', '/bin').decode().split('\n') if x and not x.startswith('.')]
passwd = [x for x in mfs('cat', '/etc/passwd').decode().split('\n') if x]
md5 = hashlib.md5(mfs('cat', '/bin/sort')).hexdigest()

def lines(*l): return list(l)
E = {}  # command index -> expected list of lines, or a callable(list)->bool
E[0] = lines()
E[1] = lines('/bin')
E[2] = lambda o: len(o) == 1 and re.fullmatch(r'X/Y\s*%d\s*Z' % len(binlist), o[0]) is not None
E[3] = lines('n1', 'n2', 'n3')
E[4] = lines('in-a', 'in-b')
E[5] = lines('or-ok', 'and-ok')
E[6] = lines('case-ok')
E[7] = lines('w1', 'w2', 'w3')
E[8] = lines('bg3-ok')
E[9] = lines(*['p'] * 8)
E[10] = lines('/')
E[11] = lines('20')
E[12] = lines()
E[13] = lines('Files are identical', 'dd-ok')
E[14] = lines('Files are identical')
E[15] = lines('Files are identical', 'cp-ok')
E[16] = lines('Tar: blocksize = 20')
E[17] = lines('Files are identical', 'Files are identical', 'tar-ok')
E[18] = lines('1')
E[19] = lines('%s  -' % md5, '%s  /bin/sort' % md5)
E[20] = sorted(passwd, reverse=True)[:2]
E[21] = sorted(set(binlist))[-3:]
E[22] = lines('500 abc')
E[23] = lines()
E[24] = lambda o: o in (['f1  f2'], ['f1', 'f2'])
E[25] = lines('sync-ok')
E[26] = lambda o: len(o) == 2 and re.fullmatch(r'0 / [1-9]\d* assertions failed', o[0]) is not None \
    and re.fullmatch(r'0 / [1-9]\d* tests failed', o[1]) is not None
E[27] = lines(" 6s: 'thisisatest'", "20s: '         thisisatest'")
E[28] = ['%d:%s' % (i + 1, l) for i, l in enumerate(passwd) if 'root' in l]
E[29] = lines(str(sum('root' in l.lower() for l in passwd)), str(sum('root' not in l for l in passwd)))
E[30] = lines('300:abcdef')
E[31] = lines('1')
E[32] = lines('880')
E[33] = lines('0', 'grep-none')
assert len(cmds) == 34 and len(E) == 34, (len(cmds), len(E))

raw = open(outf, encoding='latin1').read()
sections = []          # (header text, status flag, body lines)
for blk in re.split(r'^=== ', raw, flags=re.M)[1:]:
    head, _, body = blk.partition('\n')
    flag = ''
    # flags appended to the header: t86.py screen mode adds ' TIMEOUT'; the
    # strict harnesses (sq.py, st86.py) add ' !NOECHO', ' !ECHOBAD', ' !NOPROMPT'
    while True:
        m = re.search(r' (TIMEOUT|!NOECHO|!ECHOBAD|!NOPROMPT)$', head)
        if not m: break
        flag = (m.group(1).lstrip('!') + ' ' + flag).strip(); head = head[:m.start()]
    head = re.sub(r' \[\d+\.\d+ s\]$', '', head)
    b = [l.rstrip() for l in body.replace('\r', '').split('\n')]
    while b and not b[-1]: b.pop()
    while b and not b[0]: b.pop(0)
    if b and b[-1] == 'TIMEOUT':
        b.pop(); flag = 'TIMEOUT'
    elif b and b[-1].startswith('LINE TOO LONG'):
        b.pop(); flag = 'TOOLONG'
    # normalise number padding (wc, uniq -c pad with spaces)
    b = [l.strip() if re.fullmatch(r'\s*\d+(\s+\S.*)?\s*', l) else l for l in b]
    sections.append((head, flag, b))

fails = 0
pre = raw.split('=== ', 1)[0]
if 'BOOT TIMEOUT' in raw:
    print('BOOT: FAIL (BOOT TIMEOUT)'); fails += 1
for i, c in enumerate(cmds):
    if i >= len(sections):
        print('%2d FAIL SKIPPED   %s' % (i, c)); fails += 1; continue
    head, flag, body = sections[i]
    if head != c:
        print('%2d FAIL ORDER     %s (got: %s)' % (i, c, head)); fails += 1; continue
    if flag:
        print('%2d FAIL %-10s %s' % (i, flag, c)); fails += 1; continue
    exp = E[i]
    ok = exp(body) if callable(exp) else body == exp
    if not ok:
        print('%2d FAIL OUTPUT    %s' % (i, c))
        print('      expected: %r' % ('<rule>' if callable(exp) else exp))
        print('      got:      %r' % body); fails += 1
    else:
        print('%2d PASS           %s' % (i, c))
if len(sections) > len(cmds):
    print('EXTRA SECTIONS: %d' % (len(sections) - len(cmds))); fails += 1
print('SUMMARY %s: %d/%d PASS, %d FAIL' % (outf, len(cmds) - min(fails, len(cmds)), len(cmds), fails))
sys.exit(1 if fails else 0)

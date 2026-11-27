#!/usr/bin/env python3
"""upstream-merge.py: merge upstream ELKS into branch alci without losing Alci changes.

usage: upstream-merge.py [--upstream URL] [--branch master] [--check-only REV]

Run from the top of the Alci tree, on branch alci, with a clean work tree.

1. Fetches upstream and starts `git merge --no-commit`.
2. Conflicts: a conflicted block is resolved to upstream's side only when
   both sides are the same code once comments and blanks are removed
   (typically an Alci change taken upstream with other comments). Only the
   block is replaced, never the whole file. Any other conflict aborts the
   merge and is listed.
3. Loss check: every line Alci added since the previous upstream merge
   (git diff BASE..alci, comments and blanks ignored) must still be in the
   merged file. A file Alci changed that comes out identical to
   upstream is reported too, unless the two versions were already
   equivalent.
   Lines are compared with comments and blanks removed. Conflicts in code
   abort in step 2, so a missing line can only come from a wrong
   resolution or a merge driver: it is always reported.
4. Nothing is committed when step 2 or 3 finds something: the merge is
   left in the index for inspection (`git merge --abort` to drop it).
   When everything is clean the merge is committed; pushing, building and
   the checks of ACCEPTANCE.md (compare the built files with the previous
   build, V1 to V5 when they differ) stay separate steps.

--check-only REV: run only the loss check of step 3 on an existing merge
commit REV (its first parent is Alci, its second upstream).
"""
import re, subprocess, sys

UPSTREAM = 'https://github.com/ghaerr/elks'


def git(*a, check=True, inp=None):
    r = subprocess.run(['git'] + list(a), capture_output=True, text=True,
                       input=inp, errors='replace')
    if check and r.returncode:
        sys.exit('git %s failed:\n%s' % (' '.join(a), r.stderr))
    return r.stdout


def show(rev, path):
    r = subprocess.run(['git', 'show', '%s:%s' % (rev, path)], capture_output=True,
                       text=True, errors='replace')
    return r.stdout if r.returncode == 0 else None


def strip_code(text, path):
    """Text with comments and all blanks removed, for 'same code' tests."""
    if re.search(r'\.(c|h|S|s|inc|cpp)$', path):
        text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
        text = re.sub(r'//[^\n]*', ' ', text)
        if path.endswith(('.S', '.s', '.inc')):
            text = re.sub(r'[;!][^\n]*', ' ', text)
    elif re.search(r'(Makefile|\.mk|\.sh|\.py|config|bootopts)', path):
        text = re.sub(r'(^|\s)#[^\n]*', ' ', text)
    return re.sub(r'\s+', '', text)


def norm_line(line, path):
    return strip_code(line, path)


CONFLICT = re.compile(r'<<<<<<< [^\n]*\n(.*?)(?:\|\|\|\|\|\|\| [^\n]*\n.*?)?=======\n(.*?)>>>>>>> [^\n]*\n', re.S)


def resolve_conflicts(files):
    """Resolve conflicted blocks that differ only in comments; return unresolved."""
    bad = []
    for f in files:
        try:
            text = open(f, errors='replace').read()
        except (IsADirectoryError, FileNotFoundError):
            bad.append((f, 'not a plain file conflict (deleted or renamed)'))
            continue
        blocks = CONFLICT.findall(text)
        if not blocks:
            bad.append((f, 'no conflict markers (binary, delete or rename conflict)'))
            continue
        hard = [i for i, (o, t) in enumerate(blocks) if strip_code(o, f) != strip_code(t, f)]
        if hard:
            bad.append((f, '%d of %d blocks differ in code' % (len(hard), len(blocks))))
            continue
        open(f, 'w').write(CONFLICT.sub(lambda m: m.group(2), text))
        git('add', f)
        print('resolved %s: %d block(s), comments only, upstream text taken' % (f, len(blocks)))
    return bad


def loss_check(base, ours, theirs, merged):
    """List Alci lines missing from the merge. merged=None means the index."""
    problems, review = [], []
    files = [f for f in git('diff', '--name-only', base, ours).splitlines() if f]
    for f in files:
        m = show(merged, f) if merged else show('', f)  # ':path' is the index
        o, t, b = show(ours, f), show(theirs, f), show(base, f)
        if o is None:
            continue            # Alci deleted the file
        if m is None:
            problems.append((f, 'file missing after the merge'))
            continue
        if m == t and o != t and strip_code(o, f) != strip_code(t, f):
            problems.append((f, 'merged file is upstream\'s, Alci version gone'))
        mset = set(norm_line(l, f) for l in m.splitlines())
        tset = set(norm_line(l, f) for l in (t or '').splitlines())
        added = [l for l in git('diff', '-U0', base, ours, '--', f).splitlines()
                 if l.startswith('+') and not l.startswith('+++')]
        lost = []
        for l in added:
            n = norm_line(l[1:], f)
            if n and n not in mset:
                lost.append(l[1:].strip())
        if lost:
            # lines upstream also had in its new version cannot be lost; any
            # other missing line is: conflicts in code abort before this
            lost = [l for l in lost if norm_line(l, f) not in tset]
        if lost:
            problems.append((f, '%d Alci line(s) missing, e.g. %r' % (len(lost), lost[0][:70])))
    return problems, review


def main():
    a = sys.argv[1:]
    if '--check-only' in a:
        rev = a[a.index('--check-only') + 1]
        ours, theirs = rev + '^1', rev + '^2'
        base = git('merge-base', ours, theirs).strip()
        p, r = loss_check(base, ours, theirs, rev)
        for f, why in p:
            print('LOST    %s: %s' % (f, why))
        for f, why in r:
            print('REVIEW  %s: %s' % (f, why))
        print('LOSS CHECK %s' % ('FAILED' if p else 'OK' + (' (review listed)' if r else '')))
        sys.exit(1 if p else 0)
    url = a[a.index('--upstream') + 1] if '--upstream' in a else UPSTREAM
    br = a[a.index('--branch') + 1] if '--branch' in a else 'master'
    if git('status', '--porcelain', '--untracked-files=no').strip():
        sys.exit('work tree not clean')
    git('fetch', url, br)
    theirs = git('rev-parse', 'FETCH_HEAD').strip()
    ours = git('rev-parse', 'HEAD').strip()
    n = int(git('rev-list', '--count', 'HEAD..' + theirs).strip())
    if n == 0:
        print('nothing new upstream')
        return
    base = git('merge-base', ours, theirs).strip()
    print('upstream %s: %d new commit(s), previous base %s' % (theirs[:9], n, base[:9]))
    git('merge', '--no-commit', '--no-ff', theirs, check=False)
    conflicted = [f for f in git('diff', '--name-only', '--diff-filter=U').splitlines() if f]
    bad = resolve_conflicts(conflicted)
    if bad:
        for f, why in bad:
            print('CONFLICT %s: %s' % (f, why))
        git('merge', '--abort')
        print('MERGE ABORTED: conflicts in code, resolve by hand or report')
        sys.exit(2)
    problems, review = loss_check(base, ours, theirs, None)
    for f, why in problems:
        print('LOST    %s: %s' % (f, why))
    for f, why in review:
        print('REVIEW  %s: %s' % (f, why))
    if problems or review:
        print('MERGE NOT COMMITTED: inspect the index, then commit or `git merge --abort`')
        sys.exit(3)
    git('-c', 'user.name=proxybarrio', '-c', 'user.email=proxybarrio@users.noreply.github.com',
        'commit', '--no-edit', '-q')
    print('MERGED %s; next: build, compare the built files with the previous build, checks' %
          git('rev-parse', '--short', 'HEAD').strip())


if __name__ == '__main__':
    main()

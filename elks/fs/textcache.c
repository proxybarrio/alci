/*
 * Text cache (ALCI)
 *
 * When the last process using a program's text segment exits or execs
 * another program, the segment is kept here instead of being freed, with a
 * reference to the program's inode. The next exec of the same file takes it
 * back, so starting a program again reads, decompresses and copies no
 * text at all. The cache only holds memory nobody else wants: when an
 * allocation fails, the oldest entry is freed and the allocation retried
 * (seg_free_get in malloc.c). An entry is dropped when its file is opened
 * for writing (permission), when its file system is unmounted, and when
 * the file's size or time has changed.
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/stat.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>

#ifdef CONFIG_EXEC_TEXT_CACHE

#define TC_SIZE     4           /* programs kept, newest first */

static struct text_entry {
    struct inode *inode;
    segment_s *seg;
    __u32 size;
    __u32 mtime;
} tc[TC_SIZE];

/* remove entry n, closing the gap; returns the entry's segment and inode */
static void tc_remove(int n, segment_s **seg, struct inode **inode)
{
    *seg = tc[n].seg;
    *inode = tc[n].inode;
    for (; n < TC_SIZE - 1; n++)
        tc[n] = tc[n + 1];
    tc[TC_SIZE - 1].seg = 0;
    tc[TC_SIZE - 1].inode = 0;
}

static void tc_free(int n)
{
    segment_s *seg;
    struct inode *inode;

    tc_remove(n, &seg, &inode);
    seg_put(seg);
    iput(inode);
}

/* release a text segment: keep it if this was its last user */
void text_cache_put(segment_s *seg, struct inode *inode)
{
    int n;

    if (seg->ref_count != 1 || !inode || !S_ISREG(inode->i_mode)
            || seg->all.prev == NULL) {         /* not ours to keep */
        seg_put(seg);
        return;
    }
    if (tc[TC_SIZE - 1].seg)
        tc_free(TC_SIZE - 1);
    for (n = TC_SIZE - 1; n > 0; n--)
        tc[n] = tc[n - 1];
    tc[0].seg = seg;                            /* takes over the reference */
    tc[0].inode = inode;
    tc[0].size = inode->i_size;
    tc[0].mtime = inode->i_mtime;
    inode->i_count++;
}

/* exec: take back the text of this file, with its reference, or NULL */
segment_s *text_cache_take(struct inode *inode)
{
    int n;
    segment_s *seg;
    struct inode *ino;

    for (n = 0; n < TC_SIZE && tc[n].seg; n++) {
        if (tc[n].inode == inode) {
            if (tc[n].size != inode->i_size || tc[n].mtime != inode->i_mtime) {
                tc_free(n);                     /* the file has changed */
                return 0;
            }
            tc_remove(n, &seg, &ino);
            iput(ino);
            return seg;
        }
    }
    return 0;
}

/* the file is about to be written */
void text_cache_drop(struct inode *inode)
{
    int n;

    for (n = 0; n < TC_SIZE && tc[n].seg; n++) {
        if (tc[n].inode == inode) {
            tc_free(n);
            return;
        }
    }
}

/* the file system on dev is about to be unmounted */
void text_cache_flush(kdev_t dev)
{
    int n = 0;

    while (n < TC_SIZE && tc[n].seg) {
        if (tc[n].inode->i_dev == dev)
            tc_free(n);
        else n++;
    }
}

/* memory is short: free the oldest entry; 0 when the cache is empty */
int text_cache_evict(void)
{
    int n;

    for (n = TC_SIZE - 1; n >= 0; n--) {
        if (tc[n].seg) {
            tc_free(n);
            return 1;
        }
    }
    return 0;
}

#endif /* CONFIG_EXEC_TEXT_CACHE */

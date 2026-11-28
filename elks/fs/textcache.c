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
 *
 * Data copies: a program whose initialized data is LZSA1-compressed would
 * decode it again at each start, even with its text in memory (about 37
 * cycles a byte on the 8088: 0.11 s for 14 KB). After decoding, exec keeps
 * a copy of the decoded data, before relocation, in memory nobody wants
 * (seg_alloc_spare: never by pushing anything out), and the next start of
 * the same file copies it instead. The copies are given back first when
 * memory is short, and dropped with the text entries of their file.
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/stat.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/memory.h>

#ifdef CONFIG_EXEC_TEXT_CACHE

#define TC_SIZE     4           /* programs kept, newest first */

static struct text_entry {
    struct inode *inode;
    segment_s *seg;
    __u32 size;
    __u32 mtime;
} tc[TC_SIZE], dc[TC_SIZE];     /* texts kept; data copies (size: of the file) */

static void dc_free(int n)
{
    segment_s *seg = dc[n].seg;
    struct inode *inode = dc[n].inode;

    for (; n < TC_SIZE - 1; n++)
        dc[n] = dc[n + 1];
    dc[TC_SIZE - 1].seg = 0;
    dc[TC_SIZE - 1].inode = 0;
    seg_put(seg);
    iput(inode);
}

static void dc_drop(struct inode *inode)
{
    int n;

    for (n = 0; n < TC_SIZE && dc[n].seg; n++) {
        if (dc[n].inode == inode) {
            dc_free(n);
            return;
        }
    }
}

/* exec: the kept data copy of this file, len bytes, or NULL */
segment_s *data_copy_get(struct inode *inode, size_t len)
{
    int n;

    for (n = 0; n < TC_SIZE && dc[n].seg; n++) {
        if (dc[n].inode == inode) {
            if (dc[n].size != inode->i_size || dc[n].mtime != inode->i_mtime
                    || (dc[n].seg->size << 4) < len) {
                dc_free(n);                     /* the file has changed */
                return 0;
            }
            return dc[n].seg;
        }
    }
    return 0;
}

/* exec: keep a copy of the len bytes of decoded data at src:0 */
void data_copy_keep(struct inode *inode, seg_t src, size_t len)
{
    int n;
    segment_s *seg;

    if (!len || !inode || !S_ISREG(inode->i_mode))
        return;
    dc_drop(inode);
    seg = seg_alloc_spare((len + 15) >> 4, SEG_FLAG_KEPT);
    if (!seg)
        return;                                 /* no spare memory: none */
    fmemcpyw(0, seg->base, 0, src, (len + 1) >> 1);
    if (dc[TC_SIZE - 1].seg)
        dc_free(TC_SIZE - 1);
    for (n = TC_SIZE - 1; n > 0; n--)
        dc[n] = dc[n - 1];
    dc[0].seg = seg;
    dc[0].inode = inode;
    dc[0].size = inode->i_size;
    dc[0].mtime = inode->i_mtime;
    inode->i_count++;
}

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
    seg->flags = (seg->flags & ~SEG_FLAG_TYPE) | SEG_FLAG_KEPT;
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
            seg->flags = (seg->flags & ~SEG_FLAG_TYPE) | SEG_FLAG_CSEG;
            return seg;
        }
    }
    return 0;
}

/* the file is about to be written */
void text_cache_drop(struct inode *inode)
{
    int n;

    dc_drop(inode);
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

    while (n < TC_SIZE && dc[n].seg) {
        if (dc[n].inode->i_dev == dev)
            dc_free(n);
        else n++;
    }
    n = 0;
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

    for (n = TC_SIZE - 1; n >= 0; n--) {        /* data copies first */
        if (dc[n].seg) {
            dc_free(n);
            return 1;
        }
    }
    for (n = TC_SIZE - 1; n >= 0; n--) {
        if (tc[n].seg) {
            tc_free(n);
            return 1;
        }
    }
    return 0;
}

#endif /* CONFIG_EXEC_TEXT_CACHE */

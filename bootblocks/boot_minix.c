//------------------------------------------------------------------------------
// MINIX Boot Block (first block of the filesystem)
// Sourced from DEV86 bootblocks
// Reworked for ELKS to load directly the kernel without any helper
// Sector 2 : boot loader
//------------------------------------------------------------------------------

#include "linuxmt/config.h"
#include "minix.h"

// Global constants

#define LOADSEG DEF_INITSEG
#define OPTSEG	SEG_OPTSEG             // bootopts copied here

// Global variables

// Uninitialized data is not set to zero by default
// as the context is not a user process
// but code loaded in uninitialized memory

static byte_t sb_block [BLOCK_SIZE];  // super block block buffer
#define sb_data	((struct super_block *)sb_block)

extern char linux[], bootopts[];      // boot_sect.S (ALCI)
static int i_now;
static int i_boot;
static int ib_last = -1;             // inode block in i_block (ALCI)
static unsigned f_seg;               // segment of the next block (ALCI)
static int ib_first;                 // inode first block

static byte_t i_block [BLOCK_SIZE];  // inode block buffer
static struct inode_s * i_data;      // current inode structure

static byte_t z_block [LEVEL_MAX] [BLOCK_SIZE];  // zone block buffer

static unsigned f_left;              // blocks left to read (ALCI)

static byte_t d_dir [BLOCK_SIZE];    // root directory buffer


//------------------------------------------------------------------------------

// Helpers from boot sector

void except (char code);

void puts (const char * s);

int seg_data ();

void disk_read_blk (unsigned block, const int count, const byte_t *buf, const int seg);

void run_prog ();
void zone_load (zone_nr z, unsigned off, unsigned seg);
extern unsigned r_n;

int strcmp (const char * s, const char * d);
static void load_super ();
static void load_inode ();
static void load_zone (int level, zone_nr * z_start, zone_nr * z_end);
static void load_file ();


//------------------------------------------------------------------------------

#define seg_data()	__builtin_ia16_near_data_segment ()
#define _MK_FP(seg,off)	((void __far *)((((unsigned long)(seg)) << 16) | (off)))

//------------------------------------------------------------------------------

// This must occur right at the start of the payload.  Currently this is done
// by specifying -fno-toplevel-reorder in the Makefile, which forces GCC to
// output all functions, variables, and __asm's in the same order as in the
// source code.  FIXME: find a better way.
// REALLY running out of space here [HS] 805/2023

void load_prog ()
{
	// Avoid reuse of an old copy of /bootopts in memory
	// if we're rebooting with no /bootopts
	int __far *optseg = _MK_FP(OPTSEG, 0);
	int zero = 0;
	__asm__ ("" : "+a" (zero));	// a zero in AX: shorter stores than immediates
	*optseg = i_boot = i_now = r_n = zero;

	load_super();
	load_file ();

	for (int d = 0; d < BLOCK_SIZE /*(int)i_data->i_size*/; d += DIRENT_SIZE) {
		if (!strcmp ((char *)(d_dir + 2 + d), linux+1)) {
			i_boot = i_now = (*(int *)(d_dir + d)) - 1;
			if (i_boot == -1) continue;
			puts (linux);
			f_seg = LOADSEG;
			load_file();
			continue;
		}
		if (!strcmp ((char *)(d_dir + 2 + d), bootopts)) {
			i_now = (*(int *)(d_dir + d)) - 1;
			if (i_now != -1) {
				f_seg = OPTSEG;
				//puts("opts ");
				load_file();
			}
			continue;
		}
	}
	if (i_boot > 0)	// 'linux' may be deleted (i_boot = -1) or non-existent (=0)
		run_prog();
}

//------------------------------------------------------------------------------

// strcmp: boot_sect.S (ALCI)

//------------------------------------------------------------------------------

static void load_super ()
{
	disk_read_blk (1, 1, sb_block, seg_data ());	/* cheat and read only first sector */

	/*
	if (sb_data->s_log_zone_size) {
		//log_err ("zone size");
		err = -1;
		break;
	}
	*/

	ib_first = 2 + sb_data->s_imap_blocks + sb_data->s_zmap_blocks;

	/*
	if (sb_data->s_magic == SUPER_MAGIC) {
		dir_32 = 0;
	} else if (sb_data->s_magic == SUPER_MAGIC2) {
		dir_32 = 1;
	} else {
		//log_err ("super magic");
		err = -1;
	}
	*/
}

//------------------------------------------------------------------------------

static void load_inode ()
{
	// Compute inode block and load if not cached

	int ib = ib_first + i_now / INODES_PER_BLOCK;
	if (ib != ib_last)	// the root, /bootopts and /linux share a block (ALCI)
		disk_read_blk (ib_last = ib, 2, i_block, seg_data ());

	// Get inode data

	i_data = (struct inode_s *) i_block + i_now % INODES_PER_BLOCK;
}

//------------------------------------------------------------------------------

static void load_zone (int level, zone_nr * z_start, zone_nr * z_end)
{
	for (zone_nr * z = z_start; z < z_end; z++) {
		if (level == 0) {
			// 64K-aligned segment and offset: a read stops at 64K (DMA)
			if (!f_left) break;
			zone_load (*z, f_seg << 4, f_seg & 0xf000);
			f_seg += BLOCK_SIZE >> 4;
			f_left--;
		} else {
			int next = level - 1;
			disk_read_blk (*z, 2, z_block [next], seg_data ());
			load_zone (next, (zone_nr *) z_block [next], (zone_nr *) (z_block [next] + BLOCK_SIZE));
		}
	}
}

//------------------------------------------------------------------------------

static void load_file ()
{
	load_inode ();
	struct inode_s * ip = i_data;	// one load of i_data (ALCI)

	if (!i_now) {			// the root directory: its first block
		disk_read_blk (ip->i_zone [0], 2, d_dir, seg_data ());
		return;
	}
	f_left = ((unsigned long) ip->i_size + BLOCK_SIZE - 1) >> 10;

	// Direct zones
	load_zone (0, &(ip->i_zone [ZONE_IND_L0]), &(ip->i_zone [ZONE_IND_L1]));

	// Indirect zones
	if (f_left)
		load_zone (1, &(ip->i_zone [ZONE_IND_L1]), &(ip->i_zone [ZONE_IND_L2]));
	zone_load (0, 0, 0);	// read the last pending run

	// Double-indirect zones
	//load_zone (2, &(i_data->i_zone [ZONE_IND_L2]), &(i_data->i_zone [ZONE_IND_END]));
}

//------------------------------------------------------------------------------

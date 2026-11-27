/*
 * gembench: the D2 GUI benchmark as a GEM application (Gem-elks), in
 * alci/tests/guibench.
 *
 * The same operations as nxbench and `ctk -G`, timed in ms and written
 * to standard output (redirect it to a file: the screen is GEM's). AES
 * calls go to the GEM server and return when it has done them; a
 * WM_REDRAW for this window is handled before the time is taken; the
 * desktop background is drawn by the server itself. Run with a desktop
 * that has no window open (gemdesk would redraw its own windows later,
 * outside the time).
 *
 * Copyright (c) 2026 the Alci contributors; GPL-2.0 like Gem-elks.
 */
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>

#include "aes.h"

#define NMAX 4
static WORD N = NMAX;		/* rounds; GUIBENCH_N=1 for a slow machine */

static WORD handle, wh, wkx, wky, wkw, wkh, msg[8];
static WORD work_in[11], work_out[57];
static BYTE title[] = " Bench ";
static BYTE line[] =
	"The quick brown fox jumps over the lazy dog 0123456789 ABCDE";

static unsigned long
ms(VOID)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec % 100000UL) * 1000UL + tv.tv_usec / 1000;
}

/* wait until the screen has changed and then stayed the same for 3 s
   (the desktop drawn), reading 256 bytes of video memory (VSEG, b800
   by default) every 50 ms, as
   vwatch does: on a 640 KB 8088 GEM leaves no room to start this program
   once it is up, so it is started before GEM and waits */
static VOID
settle_screen(VOID)
{
	static unsigned char old[256];
	const char *e = getenv("VSEG");	/* a000 for EGA and VGA */
	const unsigned char __far *v = (const unsigned char __far *)
		((unsigned long) (e ? strtoul(e, NULL, 16) : 0xb800UL) << 16);
	unsigned long last = ms();
	WORD i, n, changed = 0;

	for (i = 0; i < 256; ++i)
		old[i] = v[i << 6];
	for (;;) {
		usleep(50000);
		for (n = 0, i = 0; i < 256; ++i)
			if (v[i << 6] != old[i]) {
				old[i] = v[i << 6];
				++n;
			}
		if (n >= 3) {
			last = ms();
			changed = 1;
		} else if (changed && ms() - last >= 3000UL) {
			return;
		}
	}
}

static VOID
fill(WORD x, WORD y, WORD w, WORD h)
{
	WORD xy[4];
	xy[0] = x; xy[1] = y; xy[2] = x + w - 1; xy[3] = y + h - 1;
	vswr_mode(handle, 1);
	vsf_interior(handle, 1);
	vsf_color(handle, 0);
	vr_recfl(handle, xy);
}

/* handle every message waiting (WM_REDRAW: clear the work area) */
static VOID
drain(VOID)
{
	UWORD ev, mx, my, mb, ks, kr, br;
	for (;;) {
		ev = evnt_multi(MU_MESAG | MU_TIMER, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			(LPVOID) msg, 0, 0, &mx, &my, &mb, &ks, &kr, &br);
		if (!(ev & MU_MESAG))
			return;
		if (msg[0] == WM_REDRAW && msg[3] == wh) {
			wind_update(1);
			graf_mouse(M_OFF, (LPVOID) 0);
			fill(msg[4], msg[5], msg[6], msg[7]);
			graf_mouse(M_ON, (LPVOID) 0);
			wind_update(0);
		}
	}
}

int
main(int argc, char **argv)
{
	unsigned long t, topen = 0, tclose = 0, tmove = 0, tline = 0,
		tfull = 0, tscreen = 0, tchar = 0;
	WORD i, k, sw, sh, cw, ch, bw, bh, x, y, w, h, rows;
	WORD dx, dy, dw, dh;

	(void) chdir("/lib/gemsys");
	if (getenv("GUIBENCH_N"))
		N = atoi(getenv("GUIBENCH_N"));
	if (N < 1 || N > NMAX)
		N = NMAX;
	if (argc > 1 && argv[1][1] == 'w') {
		settle_screen();
		argc = 1;
	}
	/* wait for the server; -q: and for its desktop, then quit (the
	   time of the whole command is the start time of the desktop) */
	for (i = 0; !aescheck(); ++i) {
		if (i == 600) {
			fprintf(stderr, "gembench: no GEM server\n");
			return 1;
		}
		usleep(50000);
	}
	if (appl_init((LPXBUF) 0) < 0) {
		fprintf(stderr, "gembench: no GEM server\n");
		return 1;
	}
	for (i = 0; i < 10; i++)
		work_in[i] = 1;
	work_in[10] = 2;
	v_opnvwk(work_in, &handle, work_out);
	if (argc > 1) {
		/* the server names its desktop client "DESKTOP" */
		for (i = 0; i < 600 && appl_find((LPBYTE) "DESKTOP ") < 0; ++i)
			evnt_timer(50, 0);
		evnt_timer(0, 0);
		wind_update(1); wind_update(0);
		printf("ready %lu\n", ms());
		v_clsvwk(handle);
		appl_exit();
		return 0;
	}
	sw = work_out[0] + 1;
	sh = work_out[1] + 1;
	graf_handle(&cw, &ch, &bw, &bh);
	wind_get(0, WF_WORKXYWH, &dx, &dy, &dw, &dh);	/* below the menu bar */
	vst_color(handle, 1);
	vswr_mode(handle, 1);
	w = sw * 3 / 4;
	h = sh * 2 / 3;
	x = sw / 16;
	y = dy + sh / 8;

	for (k = 0; k < N; ++k) {
		t = ms();		/* the whole desktop below the menu bar */
		form_dial(3, 0, 0, 0, 0, dx, dy, dw, dh);
		wind_update(1); wind_update(0);
		drain();
		tscreen += ms() - t;

		t = ms();
		wh = wind_create(NAME | CLOSER | MOVER, x, y, w, h);
		wind_set(wh, WF_NAME, FP_OFF(title), FP_SEG(title), 0, 0);
		wind_open(wh, x, y, w, h);
		drain();
		topen += ms() - t;
		wind_get(wh, WF_WORKXYWH, &wkx, &wky, &wkw, &wkh);
		rows = wkh / ch;
		{	/* text is clipped to the work area, as in the others */
			WORD c[4];
			c[0] = wkx; c[1] = wky;
			c[2] = wkx + wkw - 1; c[3] = wky + wkh - 1;
			vs_clip(handle, 1, c);
		}

		t = ms();
		wind_update(1);
		graf_mouse(M_OFF, (LPVOID) 0);
		v_gtext(handle, wkx, wky + ch - 1, line);
		graf_mouse(M_ON, (LPVOID) 0);
		wind_update(0);
		tline += ms() - t;

		t = ms();
		wind_update(1);
		graf_mouse(M_OFF, (LPVOID) 0);
		for (i = 0; i < rows; ++i)
			v_gtext(handle, wkx, wky + (i + 1) * ch - 1, line);
		graf_mouse(M_ON, (LPVOID) 0);
		wind_update(0);
		tfull += ms() - t;

		t = ms();
		wind_update(1);
		graf_mouse(M_OFF, (LPVOID) 0);
		v_gtext(handle, wkx, wky + ch - 1, (LPBYTE) "a");
		graf_mouse(M_ON, (LPVOID) 0);
		wind_update(0);
		tchar += ms() - t;

		t = ms();
		wind_set(wh, WF_CURRXYWH, x + sw / 8, y + sh / 16, w, h);
		drain();
		wind_update(1); wind_update(0);
		tmove += ms() - t;

		vs_clip(handle, 0, (LPWORD) 0);
		t = ms();
		wind_close(wh);
		wind_delete(wh);
		wind_update(1); wind_update(0);
		drain();
		tclose += ms() - t;
	}
	printf("guibench gem %dx%d: screen %lu open %lu close %lu move %lu\n",
		sw, sh, tscreen / N, topen / N, tclose / N, tmove / N);
	printf("guibench gem: line %lu full %lu (%d rows) char %lu\n",
		tline / N, tfull / N, rows, tchar / N);
	/* no v_clsvwk and appl_exit: on the MartyPC 8088 they did not
	   return. The server is stopped here, by the pid the script wrote in
	   /tmp/gem.pid: while it runs there is no memory left for the script
	   to start kill */
	fflush(stdout);
	{
		char b[8];
		int fd = open("/tmp/gem.pid", O_RDONLY), n = 0;
		if (fd >= 0)
			n = read(fd, b, sizeof(b) - 1);
		if (n > 0) {
			b[n] = 0;
			kill(atoi(b), SIGKILL);
		}
	}
	_exit(0);
}

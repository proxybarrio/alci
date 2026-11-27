/*
 * nxbench: the D2 GUI benchmark as a Nano-X client (alci/tests/guibench).
 *
 * The same operations as `ctk -G` and gembench, timed in ms and printed on
 * the console after the client closes. A round trip to the server
 * (GrGetScreenInfo) after each operation waits until the server has drawn
 * it: the server and its window manager do all the drawing of frames and
 * of the background, so nothing is left for another process.
 *
 * Usage: start the server (nano-X &), then nxbench; nxbench -q waits for
 * the server and quits at once. Copyright (c) 2026 the
 * Alci contributors; GPL-2.0 like the rest of the tree.
 */
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#define MWINCLUDECOLORS
#include "nano-X.h"

#define N 4                     /* each timed operation is repeated N times */

static GR_SCREEN_INFO si;
static GR_GC_ID gc;

static unsigned long
ms(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec % 100000UL) * 1000UL + tv.tv_usec / 1000;
}

static void
sync_server(void)               /* returns when the server has done all */
{
  GR_SCREEN_INFO s;
  GrGetScreenInfo(&s);
}

static void
wait_expose(GR_WINDOW_ID w)
{
  GR_EVENT ev;
  for(;;) {
    GrGetNextEvent(&ev);
    if(ev.type == GR_EVENT_TYPE_EXPOSURE && ev.exposure.wid == w)
      return;
  }
}

static GR_WINDOW_ID
open_win(int x, int y, int w, int h)
{
  GR_WINDOW_ID id = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "Bench",
                                  GR_ROOT_WINDOW_ID, x, y, w, h, WHITE);
  GrSelectEvents(id, GR_EVENT_MASK_EXPOSURE);
  GrMapWindow(id);
  wait_expose(id);
  sync_server();
  return id;
}

int
main(int argc, char **argv)
{
  static const char line[] =
    "The quick brown fox jumps over the lazy dog 0123456789 ABCDE";
  unsigned long t, topen = 0, tclose = 0, tmove = 0, tline = 0, tfull = 0,
                tscreen = 0, tkey = 0;
  GR_FONT_ID font;
  GR_FONT_INFO fi;
  int i, k, w, h, rows;
  GR_WINDOW_ID id;

  /* wait for the server started just before; -q: then quit (the time
     of the whole command is the start time of the desktop) */
  for(i = 0; GrOpen() < 0; ++i) {
    if(i == 600) {
      fprintf(stderr, "nxbench: cannot open Nano-X\n");
      return 1;
    }
    usleep(50000);
  }
  if(argc > 1) {
    GrGetScreenInfo(&si);
    printf("ready %lu\n", ms());
    GrClose();
    return 0;
  }
  GrGetScreenInfo(&si);
  gc = GrNewGC();
  font = GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL);
  GrSetGCFont(gc, font);
  GrGetFontInfo(font, &fi);
  GrSetGCForeground(gc, BLACK);
  GrSetGCBackground(gc, WHITE);
  GrSetGCUseBackground(gc, GR_TRUE);
  /* 3/4 of the screen wide and 2/3 tall, whatever the screen */
  w = si.cols * 3 / 4;
  h = si.rows * 2 / 3;
  rows = h / fi.height;

  for(k = 0; k < N; ++k) {
    t = ms();
    GrClearWindow(GR_ROOT_WINDOW_ID, GR_FALSE);   /* the whole background */
    sync_server();
    tscreen += ms() - t;

    t = ms();
    id = open_win(si.cols / 16, si.rows / 8, w, h);
    topen += ms() - t;

    t = ms();
    GrText(id, gc, 0, 0, line, sizeof(line) - 1, GR_TFASCII | GR_TFTOP);
    sync_server();
    tline += ms() - t;

    t = ms();
    for(i = 0; i < rows; ++i)
      GrText(id, gc, 0, i * fi.height, line, sizeof(line) - 1,
             GR_TFASCII | GR_TFTOP);
    sync_server();
    tfull += ms() - t;

    /* response: one character drawn and on the screen */
    t = ms();
    GrText(id, gc, 0, 0, "a", 1, GR_TFASCII | GR_TFTOP);
    sync_server();
    tkey += ms() - t;

    t = ms();
    GrMoveWindow(id, si.cols / 16 + si.cols / 8, si.rows / 8 + si.rows / 16);
    sync_server();
    tmove += ms() - t;

    t = ms();
    GrDestroyWindow(id);
    sync_server();
    tclose += ms() - t;
  }
  GrClose();
  printf("guibench nanox %dx%d: screen %lu open %lu close %lu move %lu\n",
         si.cols, si.rows, tscreen / N, topen / N, tclose / N, tmove / N);
  printf("guibench nanox: line %lu full %lu (%d rows) char %lu\n",
         tline / N, tfull / N, rows, tkey / N);
  return 0;
}

/*
 * Video routines used by the SvarDOS installer
 * Copyright (C) 2016 Mateusz Viste
 *
 * All video routines output data directly to VRAM, and they all do that
 * to both B8000:0000 and B0000:0000 areas simulteanously, for compatibility
 * with all possible video adapters in all possible modes.
 */

#include <dos.h>
#include "video.h" /* include self for control */

/* pointer to the VGA/MDA screen */
static unsigned short far *scr = (unsigned short far *)0xB8000000L;

void video_clear(unsigned short attr, int offset, int offsetend) {
  offsetend += 2000;
  while (offset < offsetend) scr[offset++] = attr;
}

/* inits screen, returns 0 for color mode, 1 for mono */
int video_init(void) {
  union REGS r;
  int monoflag;
  /* get current video mode to detect color (7 = mono, anything else is color) */
  r.h.ah = 0x0F;
  int86(0x10, &r, &r);
  /* set the monoflag to detected value and prepare the next mode we will set */
  if (r.h.al == 7) {
    monoflag = 1;
    r.h.al = 7; /* 80x25 2 colors (MDA / Hercules) */
    scr = (unsigned short far *)0xB0000000L;
  } else if (r.h.al == 2) { /* 80x25 grayscale */
    monoflag = 1;
    r.h.al = 2; /* 80x25 grayscale */
    scr = (unsigned short far *)0xB8000000L;
  } else {
    monoflag = 0;
    r.h.al = 3; /* 80x25 16 colors */
    scr = (unsigned short far *)0xB8000000L;
  }
  /* (re)set video mode to be sure what we are dealing with */
  r.h.ah = 0;
  int86(0x10, &r, &r);
  return(monoflag);
}

void video_putchar(int y, int x, unsigned short attr, int c) {
  scr[(y << 6) + (y << 4) + x] = attr | c;
}

void video_putcharmulti(int y, int x, unsigned short attr, int c, int repeat, int step) {
  int offset = (y << 6) + (y << 4) + x;
  while (repeat-- > 0) {
    scr[offset] = attr | c;
    offset += step;
  }
}

void video_putstring(int y, int x, unsigned short attr, const char *s, int maxlen) {
  if (x < 0) { /* means 'center out' */
    int slen;
    for (slen = 0; s[slen] != 0; slen++); /* faster than strlen() */
    x = 40 - (slen >> 1);
  }
  x += (y << 6) + (y << 4); /* I use x as an offset now */
  while ((*s != 0) && (maxlen-- != 0)) {
    scr[x++] = attr | *s;
    s++;
  }
}

void video_putstringfix(int y, int x, unsigned short attr, const char *s, int w) {
  x += (y << 6) + (y << 4); /* I use x as an offset now */
  while (w-- > 0) {
    scr[x++] = attr | *s;
    if (*s != 0) s++;
  }
}

void video_movecursor(int y, int x) {
  union REGS r;
  r.h.ah = 2;
  r.h.bh = 0;
  r.h.dh = y;
  r.h.dl = x;
  int86(0x10, &r, &r);
}

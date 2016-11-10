/*
 * Video routines used by the Svarog386 installer
 * Copyright (C) 2016 Mateusz Viste
 *
 * All video routines output data directly to VRAM, and they all do that
 * to both B8000:0000 and B0000:0000 areas simulteanously, for compatibility
 * with all possible video adapters in all possible modes.
 */

#include <dos.h>
#include "video.h" /* include self for control */

/* pointers to VGA/MDA screens */
static unsigned short far *vga = (unsigned short far *)0xB8000000L;
static unsigned short far *mda = (unsigned short far *)0xB0000000L;

void video_clear(unsigned short attr, int offset) {
  int x;
  for (x = offset; x < 2000; x++) {
    vga[x] = attr;
    mda[x] = attr;
  }
}

void video_putchar(int y, int x, unsigned short attr, int c) {
  int offset = (y << 6) + (y << 4) + x;
  vga[offset] = attr | c;
  mda[offset] = attr | c;
}

void video_putcharmulti(int y, int x, unsigned short attr, int c, int repeat, int step) {
  int offset = (y << 6) + (y << 4) + x;
  while (repeat-- > 0) {
    vga[offset] = attr | c;
    mda[offset] = attr | c;
    offset += step;
  }
}

void video_putstring(int y, int x, unsigned short attr, char *s) {
  x += (y << 6) + (y << 4); /* I use x as an offset now */
  while (*s != 0) {
    unsigned short b = attr | *s;
    vga[x] = b;
    mda[x] = b;
    x++;
    s++;
  }
}

void video_putstringfix(int y, int x, unsigned short attr, char *s, int w) {
  x += (y << 6) + (y << 4); /* I use x as an offset now */
  while (w-- > 0) {
    unsigned short b;
    b = attr | *s;
    if (*s != 0) s++;
    vga[x] = b;
    mda[x] = b;
    x++;
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

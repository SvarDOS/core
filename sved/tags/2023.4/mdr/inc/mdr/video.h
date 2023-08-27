/*
 * video library - provides a few functions for mode 4 and 13h programming.
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2023 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MDR_VIDEO_H

  #define MDR_VIDEO_H

  #define VIDEO_DBUF 1

  struct video_handler {
    unsigned char far *dbuf;
    int flags;
    int mode;
    unsigned char lastmode;
  };

  /* returns 0 if no VGA has been detected, non-zero otherwise */
  int video_detectvga(void);

  /* returns 0 if no EGA has been detected, non-zero otherwise */
  int video_detectega(void);

  /* init video mode. either 0x04 for CGA or 0x13 for VGA */
  struct video_handler *video_open(int mode, int flags);

  /* reads a screen dump from file and puts it to the screen buffer */
  void video_file2screen(struct video_handler *handler, char *file);

  /* load count colors of palette from array of rgb triplets */
  void video_loadpal(const unsigned char *pal, int count, int offset);

  /* Wait until VBLANK */
  void video_waitvblank(void);

  void video_flip(struct video_handler *handler);

  /* copies line ysrc over ydst in CGA mode memory */
  void video_cgalinecopy(struct video_handler *handler, int ydst, int ysrc);

  /* clear screen using color */
  void video_cls(struct video_handler *handler, unsigned char color);

  /* renders a sprite of width and height dimensions onscreen, starting at specified x/y location
     coloffset is an offset to add to color indexes, while transp is the index of the transparent color (set to -1 if none) */
  void video_putsprite(struct video_handler *handler, unsigned char *sprite, int x, int y, int width, int height, int coloffset, int transp, int maxcol);

  /* same as video_putsprite(), but reads the sprite from a file */
  void video_putspritefromfile(struct video_handler *handler, char *file, long foffset, int x, int y, int width, int height, int coloffset, int transp, int maxcol);

  void video_close(struct video_handler *handler);

  void video_rputpixel(struct video_handler *handler, int x, int y, unsigned char col, int repeat);

  /* render a horizontal line of length len starting at x/y */
  void video_hline(struct video_handler *handler, int x, int y, int len, unsigned char color);

  /* render a vertical line of length len starting at x/y */
  void video_vline(struct video_handler *handler, int x, int y, int len, unsigned char color);

  void video_line(struct video_handler *handler, int x1, int y1, int x2, int y2, unsigned char color);

  void video_rect(struct video_handler *handler, int x, int y, int width, int height, unsigned char color);

  /* renders a rectangle on screen, at position x/y, filled with color */
  void video_rectfill(struct video_handler *handler, int x, int y, int width, int height, unsigned char color);

  void video_setpalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

  void video_getpalette(unsigned char index, unsigned char *r, unsigned char *g, unsigned char *b);
#endif

/*
 * MODE-X library (320x240 8bpp)
 *
 * This file is part of Mateusz' DOS Routines <http://mateusz.fr/mdr>
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2025 Mateusz Viste
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

/*
 * This is Mateusz Viste's mode X library. It provides functions to deal
 * with the VGA mode X ("tweaked mode") at a resolution of 320x240 pixels
 * with 256 possible colors and a virtual screen of 320x819.
 *
 * The entire virtual screen can be accessed directly at all times, but only a
 * 320x240 window is actually displayed. This window can be moved up/down with
 * vidx_setorigin() allowing for smooth vertical scrolling and page flipping.
 */

#ifndef VIDX_H
#define VIDX_H

/* initializes VGA "mode X" (320x240 8bpp), make sure you have a VGA first! */
void vidx_init(void);

/* draw a single pixel of c color at [x,y] (x = 0..319 ; y = 0..818) */
void vidx_putpixel(unsigned short x, unsigned short y, unsigned char c);

/* fill an entire scanline (row of 320 pixels) with color c */
void vidx_fillscanline(unsigned short y, unsigned char c);

/* draw a horizontal line starting at [x,y] and going l pixels left */
void vidx_hline(unsigned short x, unsigned short y, unsigned short l, unsigned char c);

/* draw a vertical line starting at [x,y] and going h pixels down */
void vidx_vline(unsigned short x, unsigned short y, unsigned short h, unsigned char c);

/* draws a w x h sprite (bitmap of 8bpp pixels) with sprite's upper left corner
 * being at [x,y]. transparent pixels are skipped. if no transparency is
 * required then set transparency to -1. */
void vidx_drawsprite(unsigned short x, unsigned short y, const unsigned char *sprite, unsigned short w, unsigned short h, int transparency);

/* reads a rectangle from the VGA memory. such rectangle can then be
 * used as a sprite with vidx_drawsprite() */
void vidx_readrect(unsigned char *buff, unsigned short x, unsigned short y, unsigned short w, unsigned short h);

/* sets the first row that is to be displayed at the top of the screen.
 * MODE X has a virtual screen size of 320x819 (ie. 819 rows). By default
 * the row 0 is the origin so the first 240 rows are being displayed (0..239).
 * By changing the origin you can slide the display window up and down. */
void vidx_setorigin(unsigned short y);

/* load an RGB tuple into the given VGA palette index. RGB values must be in
 * the VGA range, ie. 0..63. */
void vidx_setpal(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

/* closes mode X and reverts to the previous video or text mode */
void vidx_close(void);

#endif

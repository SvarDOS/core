/*
 * a few functions for mode 12h programming (640x480 4bpp)
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2022-2023 Mateusz Viste
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

#ifndef MDR_VID12_H

  #define MDR_VID12_H

  /* init video mode 12h (640x480x16c)
   * remember to call vid12_close() at exit time */
  void vid12_init(void);

  /* wait until VBLANK */
  void vid12_waitvblank(void);

  /* wait until ANY blank: either VBLANK or HBLANK */
  void vid12_waitblank(void);

  /* clear screen using color
   * this function is fastest when color is 0 or 15 */
  void vid12_cls(unsigned char color);

  /* clear a single scanline (0..479) with a solid color (0..15)
   * this function is fastest when color is 0 or 15 */
  void vid12_clrline(unsigned short line, unsigned char color);

  /* fill lines from linefirst to linelast with an 8 pixels pattern
   * linelast must be equal to or greater than linelast
   * pattern must be 8 bytes long */
  void vid12_linepat(unsigned short linefirst, unsigned short linelast, const unsigned char *pattern);

  /* deinit video mode 12h and resets to previous mode */
  void vid12_close(void);

  /* puts a pixel on screen */
  void vid12_putpixel(unsigned short x, unsigned short y, unsigned char col);

  /* draws a horizonatal line from [x1,y] to [x2,y] */
  void vid12_hline(unsigned short y, unsigned short x1, unsigned short x2, unsigned char color);

  /* write an entire scanline (640 pixels) to screen. the pixels data must be
   * a serie of 640 bytes having values in the range [0..15] */
  void vid12_putscanline(unsigned short scanline, const unsigned char *pixels);

  /* set index palette color to given R,G,B value. each R,G,B component must be
   * a 6 bit value in the range [0..63] (where 63 is the maximum luminosity) */
  void vid12_setpalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

  /*****************************
   *  VRAM TO VRAM operations  *
   *****************************/

  /* prepares VGA for a VRAM-to-VRAM copy operation */
  void vid12_vramcpy_prep(void);

  /* fast (VRAM-to-VRAM) copy of a scanline (0..479) to another scanline.
   * vid12_vramcpy_prep() must be called before and vid12_vramcpy_done must be
   * called after one or more vid12_vramcpy_scanline() calls. */
  void vid12_vramcpy_scanline(unsigned short dst, unsigned short src);

  /* sets VGA back to its normal state after VRAM-to-VRAM operations */
  void vid12_vramcpy_done(void);

#endif

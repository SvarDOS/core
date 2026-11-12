/*
 * a few functions for mode 12h programming (640x480 4bpp)
 *
 * This file is part of Mateusz' DOS Routines <http://mateusz.fr/mdr>
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2026 Mateusz Viste
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

/* mdr/vga.h covers two jobs that are not specific to mode 12h:
 * mdr_vga_latchcopy() copies a rectangle already on the screen, and
 * mdr_vga_setoverscan() sets the border color. */

/* init video mode 12h (640x480x16c)
 * remember to call vid12_close() at exit time */
void vid12_init(void);

/* wait until VBLANK */
void vid12_waitvblank(void);

/* wait until ANY blank: either VBLANK or HBLANK */
void vid12_waitblank(void);

/* clear the visible screen (rows 0..479) using color. rows 480..818 are
 * left alone because some mouse drivers store data there. use vid12_clrline()
 * to clear an off-screen row. this function is fastest when color is 0 or 15 */
void vid12_cls(unsigned char color);

/* clear a single scanline (0..818) with a solid color (0..15).
 * a higher line is ignored. this function is fastest when color is 0 or 15 */
void vid12_clrline(unsigned short line, unsigned char color);

/* fill lines from linefirst to linelast with an 8 pixels pattern.
 * lines are 0..818 and linelast must be >= linefirst. the range is clipped
 * at line 818. pattern must be 8 bytes long */
void vid12_linepat(unsigned short linefirst, unsigned short linelast, const unsigned char *pattern);

/* deinit video mode 12h and resets to previous mode */
void vid12_close(void);

/* One pixel. Performs vid12_putpix_prep(), vid12_putpix() and
 * vid12_putpix_done(). A loop of these repeats that setup on every pixel;
 * a run of pixels should use the three calls below instead.
 * x is 0..639 and y is 0..818. a coordinate outside that range is ignored. */
void vid12_putpixel(unsigned short x, unsigned short y, unsigned char col);

/* Fast pixels, write mode 2. Call vid12_putpix_prep() once, then vid12_putpix()
 * any number of times, then vid12_putpix_done().
 * Between prep and done, call only vid12_putpix() and vid12_putspan(). Any
 * other vid12 draw, including vid12_putpixel(), reprograms the VGA and
 * closes the bracket.
 * Hide the mouse cursor for that stretch. vid12_putpix_done() restores the
 * state a graphics mouse driver expects: map mask on all four planes, write
 * mode 0, set/reset off, read-map select 0, bit mask 0xFF. The other public
 * draw calls restore that same state before they return. */
void vid12_putpix_prep(void);
void vid12_putpix(unsigned short x, unsigned short y, unsigned char col);
void vid12_putpix_done(void);

/* Eight pixels in the state left by vid12_putpix_prep(). Bit 7 of mask is
 * the pixel at x, bit 0 is seven pixels to the right. An unaligned run
 * crosses two VGA bytes. Pixels past x = 639 are dropped. y is 0..818.
 * A zero mask does nothing. */
void vid12_putspan(unsigned short x, unsigned short y, unsigned char mask, unsigned char col);

/* draws a horizonatal line from [x1,y] to [x2,y].
 * y is 0..818 and x2 is clipped to 639. the call is ignored when x1 is past
 * 639 or x2 is less than x1. */
void vid12_hline(unsigned short y, unsigned short x1, unsigned short x2, unsigned char color);

/* draws a vertical line from [x,y] down l pixels.
 * x is 0..639 and y is 0..818. the line is clipped at y = 818. */
void vid12_vline(unsigned short x, unsigned short y, unsigned short l, unsigned char color);

/* write an entire scanline (640 pixels) to screen. the pixels data must be
 * a serie of 640 bytes having values in the range [0..15].
 * scanline is 0..818. a higher line is ignored. */
void vid12_putscanline(unsigned short scanline, const unsigned char *pixels);

/* sends n raw scanlines to VGA plane(s). rows are 0..818; a run that would
 * pass row 818 is clipped. a scanline is a mono bitmap of 80 bytes (640 bits)
 * and the n rows are contiguous. planemask is a bitmask that tells which
 * plane(s) the pixels have to go to, eg. planemask=5 means "plane 0 and
 * plane 2", planemask=15 is all planes. The bytes are stored as themselves. */
void vid12_rawscanlines_to_planes(unsigned char planemask, unsigned short scanline_start, unsigned short n, const void *pixels);

/* set index palette color to given R,G,B value. each R,G,B component must be
 * a 6 bit value in the range [0..63] (where 63 is the maximum luminosity) */
void vid12_setpalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

/* sets the origin of the screen (top left corner) to a non-zero VRAM offset.
 * this function can be used for some limited (0..339) vertical scrolling.
 * the offset is a byte offset in VRAM. to convert it to a scanline you have
 * to multiply the scanline by 80. */
void vid12_setorigin(unsigned short offs);


/*****************************************************************************
 *                                                                           *
 *                        FAST VRAM-TO-VRAM OPERATIONS                       *
 *                                                                           *
 * In vid12 mode the VGA is set to a resolution of 640x480 at a 4bpp depth.  *
 * This video mode does not allow for proper double buffering, but there is  *
 * enough memory on a VGA to access about 2/3 of an extra page off-screen:   *
 * total addressable VRAM is 640x819, so you have a 640x339 VRAM buffer past *
 * the visible screen where you may store scanlines, sprites, glyphs etc.    *
 * Such off-screen elements can be efficiently copied to the visible screen  *
 * using vid12_vramcpy_xxx() functions.                                      *
 *                                                                           *
 * Using a MOUSE? Important Note!                                            *
 *                                                                           *
 * If your application relies on mouse input, you need to know how mouse     *
 * drivers interact with VGA memory in mode 0x12. Several drivers store      *
 * internal data in VRAM beyond the 480th scanline:                          *
 *  - MS MOUSE 11.0 and Logitech Mouseware 6.30 use scanlines 480-483        *
 *  - CTMOUSE v1.9.1 uses scanline 507                                       *
 *                                                                           *
 * Hence to avoid unexpected behavior or graphical glitches, it's best to    *
 * avoid using scanlines 480-507 if your program calls mouse services. This  *
 * precaution helps ensure compatibility across different mouse drivers.     *
 *                                                                           *
 * Public draw calls restore the VGA registers those drivers assume (all     *
 * planes writable, write mode 0, full bit mask). Hide the cursor across     *
 * vid12_putpix_prep() .. vid12_putpix_done(): the pointer is painted in     *
 * there with whatever map mask the bracket set.                             *
 *****************************************************************************/

/* prepares VGA for a VRAM-to-VRAM copy operation */
void vid12_vramcpy_prep(void);

/* fast (VRAM-to-VRAM) copy of a scanline (0..818) to another scanline.
 * a row past 818 is ignored.
 * vid12_vramcpy_prep() must be called before and vid12_vramcpy_done must be
 * called after one or more vid12_vramcpy_scanline() calls. */
void vid12_vramcpy_scanline(unsigned short dst, unsigned short src);

/* fast copy of a block of 8 x h pixels VRAM-to-VRAM from src to dst.
 * src and dst are VRAM offsets! to calculate them use this formula:
 * offset = (y * 80) + (x / 8)
 * (works only when x is a multiple of 8) */
void vid12_vramcpy_8px(unsigned short dst, unsigned short src, unsigned short h);

/* sets VGA back to its normal state after VRAM-to-VRAM operations */
void vid12_vramcpy_done(void);

#endif

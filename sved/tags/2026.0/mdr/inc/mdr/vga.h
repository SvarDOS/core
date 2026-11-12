/*
 * VGA helpers that are not tied to one graphics mode.
 * Mode 12h lives in vid12, mode X in vidx, 320x200 in video.
 *
 * This file is part of Mateusz' DOS Routines <http://mateusz.fr/mdr>
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2026 Mateusz Viste
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

#ifndef MDR_VGA_H
#define MDR_VGA_H

/* set the overscan (border) color to a DAC palette index (0..255).
 * same numbering as video_setpalette(); vid12's 0..15 colors are remapped
 * (see vid12_setpalette()). the border is outside the drawable picture and
 * the current mode may give it zero width.
 *
 * VGA only. this writes attribute register 11h, which on EGA holds a 6-bit
 * color code rather than a DAC index. CGA's border is a different port.
 * in 16-color modes, bits 7-6 of the displayed DAC index come from the
 * color-select register (attribute index 14h); 256-color modes use the
 * whole byte. */
void mdr_vga_setoverscan(unsigned char index);

/* copy a rectangle from one place on the screen to another place on the
 * same screen. the VGA moves the pixels itself. that is the fast way to
 * scroll, keep a second page, or put back the background under a sprite:
 * each byte it moves carries 4 pixels in mode X, or 8 pixels in mode 12h.
 *
 * use it after vidx_init() or vid12_init(). the 320x200 screen from
 * video_open() stores one pixel per byte, so copy that buffer with memcpy.
 *
 * dst and src are byte offsets from the start of screen memory. the left
 * edge of each rectangle has to fall on a byte boundary:
 *   mode X   offset = y * 80 + x / 4     x and the width are multiples of 4
 *            nbytes = width / 4
 *   mode 12h offset = y * 80 + x / 8     x and the width are multiples of 8
 *            nbytes = width / 8
 * rows is the height in pixels. stride is how many bytes separate the
 * start of two consecutive rows: 80 in both modes, including when the
 * rectangle is narrower than the screen. a full row or a whole page is
 * nbytes = 80 and stride = 80.
 *
 * a zero nbytes or rows does nothing. the two rectangles must not overlap,
 * and the copy must stay inside screen memory. drawing calls made before
 * and after this one keep working. */
void mdr_vga_latchcopy(unsigned short dst, unsigned short src, unsigned short nbytes, unsigned short rows, unsigned short stride);

#endif

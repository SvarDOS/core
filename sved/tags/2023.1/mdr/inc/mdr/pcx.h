/*
 * PCX-loading routines
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2022 Mateusz Viste
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

#ifndef MDR_PCX_H
#define MDR_PCX_H

struct pcx_hdr {
  unsigned char rle;
  unsigned char bpp;
  unsigned short max_x;
  unsigned short max_y;
  unsigned short bytes_per_scanline;
  struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
  } pal[256];
};

int pcx_anal(struct pcx_hdr *h, FILE *fd, unsigned long offset, unsigned short len);
int pcx_load(void *ptr, size_t ptrsz, const struct pcx_hdr *h, FILE *fd, unsigned long offset);

/* convert img to 8bpp if needed (ie unpack 2 and 4bpp data to 8bpp) */
int pcx_convto8bpp(void *img, const struct pcx_hdr *h);

#endif

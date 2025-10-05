/*
 * PCX-loading routines
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

#ifndef MDR_PCX_H
#define MDR_PCX_H

#include <stdio.h> /* FILE */

struct pcx_hdr {
  unsigned char rle;
  unsigned char bpp;
  unsigned short max_x;
  unsigned short max_y;
  unsigned short bytes_per_scanline;
  struct mdr_pcx_palentry {
    unsigned char r;
    unsigned char g;
    unsigned char b;
  } pal[256];
};

/* analyzes the header of a PCX file and fills the pcx_hdr struct accordingly.
 * fd      must be a valid (open) file descriptor.
 * offset  is the address inside the file where the PCX data is located
 *         (usually 0, unless the file is some kind of container).
 * len     is the total length of the PCX data. len=0 means "same as file size"
 * returns 0 on success, non-zero otherwise. */
int mdr_pcx_anal(struct pcx_hdr *h, FILE *fd, unsigned long offset, unsigned long len);

/* this function should be called to load the next row of a PCX image into a
 * buffer pointed at by bufptr. you will typically want to call this function
 * h->max_y times. ptr must be at least (h->bytes_per_scanline) bytes large.
 * the pcx_hdr struct must have been produced by pcx_anal().
 * returns the amount of consumed bytes */
unsigned short mdr_pcx_loadrow(void *bufptr, const struct pcx_hdr *h, const unsigned char *src);

/* convert img to 8bpp if needed (ie unpack 2 and 4bpp data to 8bpp).
 * the conversion is performed in-place, make sure the img buffer is large
 * enough to accomodate the size of the data after conversion (ie. twice as
 * big on 4bpp source, 4x times as big on 2bpp source and 8x as big on 1bpp
 * source).
 * if rowflag is set to a non-zero value, then the routine assumes img
 * contains only a single row of pixels */
int mdr_pcx_to8bpp(void *img, const struct pcx_hdr *h, unsigned char rowflag);

#endif

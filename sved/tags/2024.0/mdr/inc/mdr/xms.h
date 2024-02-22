/*
 * XMS driver
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2022 Mateusz Viste
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

#ifndef MDR_XMS_H
#define MDR_XMS_H

struct xms_struct {
  unsigned int handle;
  long memsize; /* allocated memory size, in bytes */
};

/* checks if a XMS driver is installed, inits it and allocates a memory block of memsize K-bytes.
 * if memsize is 0, then the maximum possible block will be allocated.
 * returns the amount of allocated memory (in K-bytes) on success, 0 otherwise. */
unsigned int xms_init(struct xms_struct *xms, unsigned int memsize);

/* free XMS memory */
void xms_close(struct xms_struct *xms);

/* copies a chunk of memory from conventional memory into the XMS block.
   returns 0 on sucess, non-zero otherwise. */
int xms_push(struct xms_struct *xms, void far *src, unsigned int len, long xmsoffset);

/* copies a chunk of memory from the XMS block into conventional memory */
int xms_pull(struct xms_struct *xms, long xmsoffset, void far *dst, unsigned int len);

#endif

/*
 * XMS driver
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

#ifndef MDR_XMS_H
#define MDR_XMS_H

/* checks if a XMS driver is installed, inits it and tries to allocate a
 * memory block of memsize K-bytes.
 * if memsize is 0, then the maximum possible block will be allocated.
 * returns a non-zero XMS handle on success (0 otherwise)
 * NOTE the amount of actually allocated memory (in K-bytes) is updated in
 *      memsize. Check this carefully as the routine may provide much less
 *      memory than requested if XMS is depleted. */
unsigned short mdr_xms_init(unsigned short *memsize);

/* free XMS handle, returns 0 on success */
unsigned char mdr_xms_close(unsigned short handle);

/* copies a chunk of memory from conventional memory into the XMS block.
   returns 0 on sucess, non-zero otherwise. */
unsigned char mdr_xms_push(unsigned short handle, void far *src, unsigned long xmsoffset, unsigned short len);

/* copies a chunk of memory from the XMS block into conventional memory */
unsigned char mdr_xms_pull(unsigned short handle, void far *dst, unsigned long xmsoffset, unsigned short len);

#endif

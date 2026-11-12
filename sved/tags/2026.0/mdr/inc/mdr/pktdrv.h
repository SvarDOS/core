/*
 * Packet driver library for Watcom C
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

#ifndef MDR_PKTDRV_H
#define MDR_PKTDRV_H

/* intitializes packet driver, returns the interrupt number of the packet
 * driver used, or 0 on error.
 * pktint = interrupt number to look at (0 = automatic scan)
 * this function is mandatory before using any other of the pktdrv functions */
unsigned char mdr_pktdrv_init(unsigned char pktint);

/* registers a packet driver handle for given  ethertype (or all ethertypes if
 * value is 0). returns a handle on success, 0xffff otherwise. */
unsigned short mdr_pktdrv_accesstype(unsigned short ethertype);

/* get my own MAC addr. target MUST point to a space of at least 6 chars */
void mdr_pktdrv_getmac(unsigned char *dst, unsigned short handle);

/* returns a pointer to the frame buffer
 * the frame buffer starts with a single word (16-bit) that provides the status
 * of the buffer:
 * 0 = EMPTY
 * bit 0x8000 set = WRITE IN PROGRESS (do not touch)
 * other value = length of frame that follows, in bytes
 * you do not need to call this function more than one time. Once you have the
 * frame buffer address you can use it indefinitely, it never changes. */
void *mdr_pktdrv_getbufptr(void);

/* sends a frame of len bytes, returns 0 on success */
int mdr_pktdrv_send(const void *pkt, unsigned short len);

/* frees the given handle (protocol) of the packet driver */
void mdr_pktdrv_free(unsigned short handle);

#endif

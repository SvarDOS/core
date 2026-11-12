/*
 * ISA DMA buffer allocation
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

#ifndef MDR_DMA_H
#define MDR_DMA_H

/* allocates len bytes that do not cross a 64K physical page. ISA DMA (the
 * 8237) cannot cross that boundary. returns NULL if len is 0, if the block
 * cannot be allocated, or if no placement inside one page exists.
 * a DOS allocation is at most 65535 bytes and this call keeps a few bytes
 * of bookkeeping in the same block, so a len above about 32K may fail even
 * when that many bytes are free. free the buffer with mdr_dma_free(). */
void *mdr_dma_alloc(unsigned short len);

/* releases a buffer returned by mdr_dma_alloc(). NULL is ignored. */
void mdr_dma_free(void *p);

#endif

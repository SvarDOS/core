/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef RMODINIT_H
#define RMODINIT_H

#define RMOD_OFFSET_ENVSEG     0x08
#define RMOD_OFFSET_LEXITCODE  0x0A
#define RMOD_OFFSET_INPBUFF    0x0C
#define RMOD_OFFSET_COMSPECPTR 0x8E
#define RMOD_OFFSET_BOOTDRIVE  0x90
#define RMOD_OFFSET_ECHOFLAG   0x9F
#define RMOD_OFFSET_BATCHCHAIN 0xA0
#define RMOD_OFFSET_ROUTINE    0xA2

unsigned short rmod_install(unsigned short envsize);
unsigned short rmod_find(void);
void rmod_updatecomspecptr(unsigned short rmod_seg, unsigned short env_seg);

#endif

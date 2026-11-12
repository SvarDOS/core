/*
 * MVCOMP decompression.
 *
 * A stream is a list of little-endian 16-bit words. There is no header,
 * no stored length and no checksum. The caller knows how many compressed
 * bytes to feed. Matches are copied from bytes already written, so the
 * output itself is the window (4096 bytes back at most).
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

#ifndef MDR_MVCOMP_H
#define MDR_MVCOMP_H

/* Bytes to allocate so an asset of uncomplen plain bytes can be unpacked
 * inside that one buffer: uncomplen + uncomplen/31 + 2.
 * Copy the compressed bytes to the end of the buffer and unpack at offset 0.
 * The sum can exceed 65535, which does not fit in one real-mode segment. */
unsigned long mdr_mvcomp_bufsize(unsigned short uncomplen);

/* Unpack the MVCOMP stream at src into dst. srclen is the compressed size
 * in bytes. Returns how many bytes were written.
 *
 * A length below 2 writes nothing. The low bit of srclen is ignored. The
 * first word of a stream has a clear top nibble; any other first word
 * writes nothing.
 *
 * A literal group is written only when every raw word it names lies inside
 * srclen. A back reference copies 2 to 16 bytes from 1 to 4096 bytes before
 * the write position, one byte at a time, so the copy may repeat bytes it
 * has just stored. The stream is trusted: a reference that begins before
 * dst reads whatever already sits in front of dst.
 *
 * src and dst may be in different segments, so a picture can be unpacked
 * into video memory. They may also be the same buffer; size it with
 * mdr_mvcomp_bufsize() and place the stream at the end. Each span must fit
 * in its own segment from the given offset. The call keeps no static data.
 * The direction flag is left clear. */
unsigned short mdr_mvcomp_depack(void far *dst, const void far *src, unsigned short srclen);

#endif

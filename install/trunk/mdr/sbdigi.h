/*
 * SoundBlaster routines for DSP driving
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

#ifndef MDR_SBDIGI_H
#define MDR_SBDIGI_H

struct sbdigi_ctx;

/* initializes the SoundBlaster DSP chip
 * blaster must point to a BLASTER environment string (like "A220 I5 D1")
 * returns a pointer to a context, NULL on error
 * NOTE: DSP's state after initialization may or may not be muted, depending
 *       on the exact hardware revision. use sbdigi_spkoff() to make sure it is
 *       unmuted */
struct sbdigi_ctx *sbdigi_init(const char *blaster);

/* unmutes the SoundBlaster DSP */
void sbdigi_spkon(struct sbdigi_ctx *ctx);

/* mutes the SoundBlaster DSP */
void sbdigi_spkoff(struct sbdigi_ctx *ctx);

/* plays a short sample
 * ctx - DSP context, as returned by sbdigi_init()
 * buf - pointer to sample data (must be PCM, 8000 Hz, mono, 8-bit unsigned
 * len - length of the sample, in bytes
 * NOTES: this routine uses DMA to transfer memory. This has two implications:
 *  1. the routine will return almost immediately, while the sound is playing
 *  2. sample data must be contained in a buffer that does NOT cross a 64K page
 *     because DMA transfers are unable to cross 64K boundaries */
void sbdigi_playsample(struct sbdigi_ctx *ctx, void *buf, unsigned short len);

/* shuts down the DSP and frees context memory */
void sbdigi_quit(struct sbdigi_ctx *ctx);

#endif

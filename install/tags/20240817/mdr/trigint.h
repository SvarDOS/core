/*
 * Routines for computation of basic transcendental functions sin and cos.
 * These routines use only integers, hence they do not require an FPU nor any
 * kind of FPU emulation. Works reasonably fast even on an 8086 CPU.
 *
 * The results are computed using polynomial approximations. Their precision
 * is not expected to be ideal, but "good enough" for common usage.
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

#ifndef MDR_TRIGINT_H
#define MDR_TRIGINT_H

/* Computes the cosine value for the given radian.
 * The radian argument must be provided multiplied by 1000.
 * Returns the cosine value multiplied by 1000. */
short trigint_cos(short rad1000);

/* Computes the sine value for the given radian angle.
 * The radian argument must be provided multiplied by 1000.
 * Returns the cosine value multiplied by 1000. */
short trigint_sin(short rad1000);

#endif

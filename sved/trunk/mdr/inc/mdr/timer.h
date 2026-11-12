/*
 * High-resolution timing routines (PIT reprogramming)
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

#ifndef MDR_TIMER_H
#define MDR_TIMER_H

/* Starts the timer by reprogramming the 8253 chip from its default 18.2 Hz
 * frequency to about 1.1 kHz. It is mandatory to revert the timer to its
 * original frequency via mdr_timer_stop() before your application quits. */
void mdr_timer_init(void);

/* Fills res with the amount of microseconds that elapsed since either
 * mdr_timer_init() or mdr_timer_reset(), whichever was called last.
 * Note that the res counter wraps around approximately every 71 minutes if
 * mdr_timer_reset() is not called. */
void mdr_timer_read(unsigned long *res);

/* Reset the timer value, this can be used by the application to make sure
 * no timer wrap occurs during critical parts of your code flow */
void mdr_timer_reset(void);

/* Stops (uninstalls) the timer. This must be called before your application
 * quits, otherwise the system will likely crash. This function has a void
 * return value so that it can be registered as an atexit() procedure. */
void mdr_timer_stop(void);

#endif

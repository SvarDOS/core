/*
 * High-resolution timing routines (PIT reprogramming)
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

#ifndef MDR_TIMER_H
#define MDR_TIMER_H

/* reset the timer value, this can be used by the application to make sure
 * no timer wrap occurs during critical parts of the code flow */
void timer_reset(void);

/* This routine will stop the fast clock if it is going. It has void return
 * value so that it can be an exit procedure. */
void timer_stop(void);

/* This routine will start the fast clock rate by installing the
 * handle_clock routine as the interrupt service routine for the clock
 * interrupt and then setting the interrupt rate up to its higher speed
 * by programming the 8253 timer chip.
 * This routine does nothing if the clock rate is already set to
 * its higher rate, but then it returns -1 to indicate the error. */
void timer_init(void);

/* This routine will return the present value of the time, which is
 * read from the nowtime structure. Interrupts are disabled during this
 * time to prevent the clock from changing while it is being read. */
void timer_read(unsigned long *res);

#endif

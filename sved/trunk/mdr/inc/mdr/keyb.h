/*
 * Keyboard routines
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2023 Mateusz Viste
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

#ifndef KEYB_MDR_H
#define KEYB_MDR_H

/* waits for a keypress and return it. Extended keys are returned ORed with
 * 0x100 (example: PGUP is 0x149). */
int keyb_getkey(void);

/* poll the keyboard, and return the next input key if any, or -1 */
int keyb_getkey_ifany(void);

/* flush the keyboard buffer */
void keyb_flush(void);

#endif
/*
 * Mouse routines
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

#ifndef MDR_MOUSE_H
  #define MDR_MOUSE_H

  /* init the mouse driver (and checks for presence of mouse support at same time)
   * returns 0 if no mouse is present, and the number of buttons otherwise.
   * after initialization the mouse cursor is hidden, use mdr_mouse_show() to
   * make it visible. */
  unsigned char mdr_mouse_init(void);

  /* shows the mouse pointer */
  void mdr_mouse_show(void);

  /* hides the mouse pointer */
  void mdr_mouse_hide(void);

  /* get x/y coordinates of the mouse, and returns a bitfield with state of buttons */
  int mdr_mouse_getstate(int *x, int *y);

  /* fetches last horizontal and vertical motion since last call (in mickeys) */
  void mdr_mouse_rawmotion(int *horz, int *vert);

  /* get x/y coordinates of the mouse when the last button release occured since last check.
   * returns the id of the button pressed (1 or 2), or 0 if no event occured.
   * x,y coordinates are optional (can be NULL) */
  int mdr_mouse_fetchrelease(int *x, int *y);

  /* set graphic pointer shape. icon is 64-bytes long, two sets of 32. each set
   * of 32 bytes is organized as a 16x16 bitmap, ie. 16 rows of 16-bit values.
   * a) the first set of 16 shorts defines the background mask - that is, the
   * background will show through wherever there is a 1-bit in that data.
   * b) the second set defines the "XOR mask" - that is, the pixels matching the
   * 1-bit in this data set are toggled.
   * hotspotx and hotspoty define respectively the horizontal and vertical hot
   * spot of the pointer (default being [0,0], that is the top left corner). */
  void mdr_mouse_setcursor(const unsigned short *icon, unsigned char hotspotx, unsigned char hotspoty);

#endif

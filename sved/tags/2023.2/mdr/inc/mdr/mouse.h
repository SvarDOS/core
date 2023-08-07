/*
 * Mouse routines
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

#ifndef MDR_MOUSE_H
  #define MDR_MOUSE_H

  /* init the mouse driver (and checks for presence of mouse support at same time)
   * returns 0 if no mouse is present, and the number of buttons otherwise */
  int mouse_init(void);

  /* shows the mouse pointer */
  void mouse_show(void);

  /* hides the mouse pointer */
  void mouse_hide(void);

  /* get x/y coordinates of the mouse, and returns a bitfield with state of buttons */
  int mouse_getstate(int *x, int *y);

  /* get x/y coordinates of the mouse when the last button release occured since last check.
     returns the id of the button pressed (1 or 2), or 0 if no event occured. */
  int mouse_fetchrelease(int *x, int *y);

#endif

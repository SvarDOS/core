/* This file is part of the svarlang project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2023 Mateusz Viste
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

#include <stdlib.h> /* _psp */
#include <string.h> /* _fmemcpy() */
#include <i86.h>    /* MK_FP() */

#include "svarlang.h"

int svarlang_autoload_exepath(const char *lang) {
  unsigned short far *psp_envseg = MK_FP(_psp, 0x2c); /* pointer to my env segment field in the PSP */
  char far *myenv = MK_FP(*psp_envseg, 0);
  unsigned short len;
  char orig_ext[3];
  int res;

  /* who am i? look into my own environment, at the end of it should be my EXEPATH string */
  while (*myenv != 0) {
    /* consume a NULL-terminated string */
    while (*myenv != 0) myenv++;
    /* move to next string */
    myenv++;
  }
  myenv++; /* skip the nul terminator */

  /* check next word, if 1 then EXEPATH follows */
  if (*myenv != 1) return(-1);
  myenv++;
  if (*myenv != 0) return(-1);
  myenv++;

  /* myenv contains my full name, find end of string now */
  for (len = 0; myenv[len] != 0; len++);

  /* must be at least 5 bytes long and 4th char from end must be a dot (like "a.exe") */
  if ((len < 5) || (myenv[len - 4] != '.')) return(-1);

  /* copy extension to buffer and replace it with "lng" */
  orig_ext[0] = myenv[len - 3];
  orig_ext[1] = myenv[len - 2];
  orig_ext[2] = myenv[len - 1];

  myenv[len - 3] = 'L';
  myenv[len - 2] = 'N';
  myenv[len - 1] = 'G';

  /* try loading it now */
  res = svarlang_load(myenv, lang); /* TODO FIXME myenv is a far pointer, while svarlang_load() in small or medium memory models expects a near ptr */

  /* restore the original filename and quit */
  myenv[len - 3] = orig_ext[0];
  myenv[len - 2] = orig_ext[1];
  myenv[len - 1] = orig_ext[2];

  return(res);
}

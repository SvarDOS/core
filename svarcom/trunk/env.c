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

/*
 * routines used to manipulate the environment block
 */

#include <i86.h>
#include <string.h>

#include "env.h"

/* looks for varname in environment block and returns a far ptr to it if
 * found, NULL otherwise. varname MUST be in upper-case and MUST be terminated
 * by either a = sign or a NULL terminator */
char far *env_lookup(unsigned short env_seg, const char *varname) {
  char far *env = MK_FP(env_seg, 0);
  int i;
  for (;;) {
    /* is this it? */
    for (i = 0;; i++) {
      if ((varname[i] == '=') || (varname[i] == 0)) {
        if (env[i] == '=') return(env); /* FOUND! */
        break; /* else look for next string */
      }
      if (varname[i] != env[i]) break;
    }
    /* move env to end of current string */
    while (*env != 0) env++;
    /* if there's another trailing zero, then that is the end of environment */
    env++;
    if (*env == 0) return(NULL);
  }
}


/* returns the size, in bytes, of the allocated environment block */
unsigned short env_allocsz(unsigned short env_seg) {
  unsigned short far *mcbsz = MK_FP(env_seg - 1, 3); /* block size is a word at offset +3 in the MCB */
  return(*mcbsz * 16); /* return size in bytes, not paragraphs */
}


/* remove a variable from environment, if present. returns 0 on success, non-zero if variable not found */
int env_dropvar(unsigned short env_seg, const char *varname) {
  unsigned short blocksz, traillen;
  unsigned short len;
  char far *varptr = env_lookup(env_seg, varname);

  /* if variable not found in environment, quit now */
  if (varptr == NULL) return(-1);

  for (len = 0; varptr[len] != 0; len++); /* compute length of variable (without trailing null) */
  blocksz = env_allocsz(env_seg);           /* total environment size */
  traillen = blocksz - (FP_OFF(varptr) + len + 1); /* how much bytes are present after the variable */
  _fmemset(varptr, 0, len);               /* zero out the variable */
  if (traillen != 0) {
    _fmemmove(varptr, varptr + len + 1, traillen); /* move rest of memory */
  }
  return(0);
}


/* Writes a variable to environment block. The variable must in the form
 * "VARNAME=value". If variable is already present in the environment block,
 * then the value will be updated. If the new value is empty, then the
 * existing variable (if any) is removed.
 * VARNAME *MUST* be all-uppercase.
 *
 * This function returns:
 *   ENV_SUCCESS = success
 *   ENV_NOTENOM = not enough available space in memory block
 *   ENV_INVSYNT = invalid syntax
 */
int env_setvar(unsigned short env_seg, const char *v) {
  unsigned short envlen;
  unsigned short envfree;
  unsigned short vlen, veqpos;
  char far *env = MK_FP(env_seg, 0);

  /* remove variable from environment, if already set */
  env_dropvar(env_seg, v);

  /* compute v length and find the position of the eq sign */
  veqpos = 0xffff;
  for (vlen = 0; v[vlen] != 0; vlen++) {
    if (v[vlen] == '=') {
      if (veqpos != 0xffff) return(ENV_INVSYNT); /* equal sign is forbidden in value */
      veqpos = vlen;
    }
  }

  /* if variable empty, stop here */
  if (veqpos == vlen - 1) return(ENV_SUCCESS);

  /* compute current size of the environment */
  for (envlen = 0; env[envlen] != 0; envlen++) {
    while (env[envlen] != 0) envlen++; /* consume a string */
  }

  /* compute free space available in environment */
  envfree = env_allocsz(env_seg);
  envfree -= envlen;
  envfree -= 1; /* 1 byte for the environment's NULL terminator */

  /* do I have enough env space for the new var? */
  if (envfree < vlen + 1) return(ENV_NOTENOM);

  /* write the new variable (with its NULL terminator) to environment tail */
  _fmemcpy(env + envlen, v, vlen + 1);

  /* add the environment's NULL terminator */
  env[envlen + vlen + 1] = 0;

  return(ENV_SUCCESS);
}

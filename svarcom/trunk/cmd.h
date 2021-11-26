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

#ifndef CMD_H
#define CMD_H

#include "rmodinit.h"

/* what cmd_process may return */
enum cmd_result {
  CMD_OK,         /* command executed and succeeded */
  CMD_FAIL,       /* command executed and failed */
  CMD_NOTFOUND,   /* no such command (not an internal command) */
  CMD_CHANGED     /* command-line transformed, please reparse it */
};

/* process internal commands */
enum cmd_result cmd_process(struct rmod_props far *rmod, unsigned short env_seg, const char *cmdline, void *BUFFER, unsigned short BUFFERSZ, const struct redir_data *r);

/* explodes a command into an array of arguments where last arg is NULL.
 * if argvlist is not NULL, it will be filled with pointers that point to buff
 * locations. buff is filled with all the arguments, each argument being
 * zero-separated. buff is terminated with an empty argument to mark the end
 * of arguments.
 * returns number of args */
unsigned short cmd_explode(char *buff, const char far *s, char const **argvlist);

#endif

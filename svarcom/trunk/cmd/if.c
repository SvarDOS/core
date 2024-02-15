/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2022 Mateusz Viste
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
 * if [not] exists
 * if [not] errorlevel 1
 * if [not] string ==string (string==string and string == string works, too)
 * if [not] errorlevel == 1   <-- I do NOT support this one, even though
 *                                MSDOS 5 and 6 considers it equivalent to
 *                                IF ERRORLEVEL 1. This is a misleading and
 *                                undocumented syntax (does not actually
 *                                check for equality).
 */


#define JMP_NEXT_ARG(s) while ((*s != ' ') && (*s != 0)) s++; while (*s == ' ') s++;


static enum cmd_result cmd_if(struct cmd_funcparam *p) {
  unsigned char negflag = 0;
  unsigned short i;
  const char *s = p->cmdline + p->argoffset;

  /* help screen ONLY if /? is the only argument - I do not want to output
   * help for ex. for "if %1 == /? echo ..." */
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    nls_outputnl(35,0); /* "Performs conditional processing in batch programs." */
    outputnl("");
    nls_outputnl(35,1); /* "IF [NOT] ERRORLEVEL num command" */
    nls_outputnl(35,2); /* "IF [NOT] string1==string2 command" */
    nls_outputnl(35,3); /* "IF [NOT] EXIST filename command" */
    outputnl("");
    nls_outputnl(35,4); /* "NOT               command is executed only if condition is NOT met" */
    nls_outputnl(35,5); /* "ERRORLEVEL num    condition: last program returned an exit code >= num" */
    nls_outputnl(35,6); /* "string1==string2  condition: both strings must be equal" */
    nls_outputnl(35,7); /* "EXIST filename    condition: filename exists (wildcards accepted)" */
    nls_outputnl(35,8); /* "command           command to carry out if condition is met" */
    return(CMD_OK);
  }

  /* negation? */
  if (imatchlim(s, "NOT ", 4)) {
    negflag = 1;
    JMP_NEXT_ARG(s);
  }

  /* IF ERRORLEVEL x cmd */
  if (imatchlim(s, "ERRORLEVEL ", 11)) {
    unsigned char far *rmod_exitcode = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_LEXITCODE);
    JMP_NEXT_ARG(s);
    if (*s == 0) goto SYNTAX_ERR;
    /* convert errorlevel to an uint */
    if ((*s < '0') || (*s > '9')) {
      i = 0xffff;
    } else {
      atous(&i, s);
    }
    /* move s to command */
    JMP_NEXT_ARG(s);
    /* is errorlevel matching? */
    if (i <= *rmod_exitcode) negflag ^= 1;
    goto EXEC_S_CMD_IF_NEGFLAG_SET;
  }

  /* IF EXIST fname (or wildcard)
   * TODO: checking for a file on an empty diskette drive should NOT lead bother
   *       the user with the stupid 'retry, abort, fail' query! */
  if (imatchlim(s, "EXIST ", 6)) {
    struct DTA *dta = (void *)(0x80); /* default dta location */
    JMP_NEXT_ARG(s);
    /* copy filename to buffer */
    for (i = 0; (s[i] != ' ') && (s[i] != 0); i++) p->BUFFER[i] = s[i];
    p->BUFFER[i] = 0;
    /* move s to command */
    JMP_NEXT_ARG(s);
    if (*s == 0) goto SYNTAX_ERR; /* check now to avoid moving the diskette drive if syntax bad anyway */
    /* does file exist? */
    if (findfirst(dta, p->BUFFER, 0) == 0) negflag ^= 1;
    goto EXEC_S_CMD_IF_NEGFLAG_SET;
  }

  /* assume "IF str1==str2" and if that's not it then it's a syntax error */
  /* copy first argument to BUFF, until first '=' or space */
  for (i = 0; (s[i] != '=') && (s[i] != ' ') && (s[i] != 0); i++) p->BUFFER[i] = s[i];
  /* 1st arg cannot be empty */
  if (i == 0) goto SYNTAX_ERR;
  /* terminate buff string */
  p->BUFFER[i] = 0;
  s += i;
  while (*s == ' ') s++;
  /* should be two '=' now, otherwise it's a syntax error (equality sign is not
   * allowed in first string) */
  if ((s[0] != '=') || (s[1] != '=')) goto SYNTAX_ERR;
  /* skip all trailing equality chars (MSDOS accepts many of them, ie all
   * these are fine: "dupa==dupa", "dupa===dupa", "dupa====dupa", etc) */
  while (*s == '=') s++;
  while (*s == ' ') s++; /* skip any leading spaces */
  /* move along until space or NULL terminator, checking equality */
  for (i = 0; (p->BUFFER[i] != 0) && (p->BUFFER[i] == s[i]); i++);
  if ((p->BUFFER[i] == 0) && (s[i] == ' ')) negflag ^= 1;
  JMP_NEXT_ARG(s);
  goto EXEC_S_CMD_IF_NEGFLAG_SET;

  /* invalid syntax */
  SYNTAX_ERR:
  nls_outputnl(0,1); /* "Invalid syntax" */
  return(CMD_FAIL);

  /* let's exec command (write it to start of cmdline and parse again) */
  EXEC_S_CMD_IF_NEGFLAG_SET:
  if (*s == 0) goto SYNTAX_ERR;
  if (negflag == 0) return(CMD_OK);
  memmove((void *)(p->cmdline), s, strlen(s) + 1);  /* cmdline and s share the same memory! */
  return(CMD_CHANGED);
}

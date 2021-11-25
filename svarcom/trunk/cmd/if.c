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


static int cmd_if(struct cmd_funcparam *p) {
  unsigned char negflag = 0;
  unsigned short i;
  const char *s = p->cmdline + p->argoffset;

  /* help screen ONLY if /? is the only argument - I do not want to output
   * help for ex. for "if %1 == /? echo ..." */
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    outputnl("Performs conditional processing in batch programs.");
    outputnl("");
    outputnl("IF [NOT] ERRORLEVEL num command");
    outputnl("IF [NOT] string1==string2 command");
    outputnl("IF [NOT] EXIST filename command");
    outputnl("");
    outputnl("NOT               command is executed only if condition is NOT met");
    outputnl("ERRORLEVEL num    condition: last program returned an exit code >= num");
    outputnl("string1==string2  condition: both strings must be equal");
    outputnl("EXIST filename    condition: filename exists");
    outputnl("command           command to carry out if condition is met.");
    return(-1);
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
    JMP_NEXT_ARG(s);
    if (*s == 0) goto SYNTAX_ERR;
    /* is errorlevel matching? */
    if (i <= *rmod_exitcode) negflag ^= 1;
    if (negflag) {
      output("EXEC (TO BE IMPLEMENTED): ");
      outputnl(s);
    }
    return(-1);
  }

  /* TODO IF EXISTS fname */
  /* TODO IF str1==str2 */

  SYNTAX_ERR:

  /* invalid syntax */
  outputnl("Syntax error");

  return(-1);
}

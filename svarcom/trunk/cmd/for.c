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
 * FOR %variable IN (set) DO command [command-parameters]
 *
 * %variable    a replaceable parameter name
 * (set)        a set of one of more files. Wildcards allowed.
 * command      the command to carry out for each matched file
 * command-parameters   parameters or switches for the specified command
 *
 * To use FOR in a batch program, use %%variable instead of %variable
 *
 * the set of files must be surrounded by parenthesis, and may contain one
 * or more space-delimited wildcards. Examples:
 * (file.txt other.txt)
 * (*.exe *.com *.bat)
 * (jan*.txt 199201??.txt)
 */

/* Implementation notes:
 *
 * For cannot be nested (no "FOR ... do FOR ..." allowed)
 *
 * When executed, FOR allocates a "FOR context" to RMOD's memory, this context
 * holds the memory state of a FindNext iteration, as well as the command
 * to run on each matched file.
 *
 * This FOR context is looked at by command.c and used to provide a command
 * instead of getting the command from interactive cli or BAT file. Repeats
 * until the FindNext buffer stops matching files.
 *
 * This file only provides the help screen of FOR, as well as the FOR-context
 * initialization. Actual execution happens within command.c.
 */


/* normalizing for-loop pattern-list separators.
 * returns a space char if c is a for-pattern separator, c otherwise */
static char cmd_for_sepnorm(char c) {
  /* the list of valid delimiters has been researched and kindly shared by Robert
   * Riebisch via the ticket at https://osdn.net/projects/svardos/ticket/44058 */
  switch (c) {
    case ' ':
    case '\t':
    case ';':
    case ',':
    case '/':
    case '=':
      return(' '); /* normalize FOR-loop separators to a space */
    default: /* anything else is kept as-is */
      return(c);
  }
}


static enum cmd_result cmd_for(struct cmd_funcparam *p) {
  struct forctx *f = (void *)(p->BUFFER);
  unsigned short i;

  /* forbid nested FORs */
  if (p->rmod->forloop) {
    nls_outputnl(18,7); /* FOR cannot be nested */
    return(CMD_FAIL);
  }

  /* help screen ONLY if /? is the only argument */
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    nls_outputnl(18,0); /* "Runs a specified command for each file in a set of files" */
    outputnl("");
    nls_outputnl(18,1); /* "FOR %variable IN (set) DO command [parameters]" */
    outputnl("");
    nls_outputnl(18,2); /* "%variable    a replaceable parameter name" */
    nls_outputnl(18,3); /* "(set)        a set of one of more files. Wildcards allowed." */
    nls_outputnl(18,4); /* "command      the command to carry out for each matched file" */
    nls_outputnl(18,5); /* "parameters   parameters or switches for the specified command" */
    outputnl("");
    nls_outputnl(18,6); /* "To use FOR in a batch program, use %%variable instead of %variable" */
    return(CMD_OK);
  }

  /* clear out struct and copy command line to it */
  bzero(f, sizeof(*f));
  strcpy(f->cmd, p->cmdline);

  /* locate the %varname (single char) */
  i = p->argoffset;
  while (f->cmd[i] == ' ') i++;
  if ((f->cmd[i] != '%') || (f->cmd[i+1] == ' ') || (f->cmd[i+2] != ' ')) goto INVALID_SYNTAX;
  f->varname = f->cmd[i+1];
  i += 3;

  /* look (and skip) the "IN" part */
  while (f->cmd[i] == ' ') i++;
  if (((f->cmd[i] & 0xDF) != 'I') && ((f->cmd[i+1] & 0xDF) != 'N') && (f->cmd[i+2] != ' ')) goto INVALID_SYNTAX;
  i += 3;

  /* look for patterns start */
  while (f->cmd[i] == ' ') i++;
  if (f->cmd[i] != '(') goto INVALID_SYNTAX;
  i++;
  while (cmd_for_sepnorm(f->cmd[i]) == ' ') i++;
  f->nextpat = i;
  /* look for patterns end and normalize all separators to a space */
  while ((f->cmd[i] != ')') && (f->cmd[i] != 0)) {
    f->cmd[i] = cmd_for_sepnorm(f->cmd[i]);
    i++;
  }
  if (f->cmd[i] != ')') goto INVALID_SYNTAX;
  f->cmd[i++] = 0; /* terminate patterns and move to next field */

  /* look (and skip) the "DO" part */
  while (f->cmd[i] == ' ') i++;
  if (((f->cmd[i] & 0xDF) != 'D') && ((f->cmd[i+1] & 0xDF) != 'O') && (f->cmd[i+2] != ' ')) goto INVALID_SYNTAX;
  i += 3;
  while (f->cmd[i] == ' ') i++;

  /* rest is the exec string */
  f->exec = i;

  /* alloc memory for the forctx context and copy f to it */
  p->rmod->forloop = rmod_fcalloc(sizeof(*f), p->rmod->rmodseg, "SVFORCTX");
  if (p->rmod->forloop == NULL) {
    nls_outputnl_doserr(8);
    return(CMD_FAIL);
  }
  _fmemcpy(p->rmod->forloop, f, sizeof(*f));

  return(CMD_OK);

  INVALID_SYNTAX:
  nls_outputnl(0,1); /* "Invalid syntax" */
  return(CMD_FAIL);
}

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
 * goto label
 *
 * if label does not exist in the currently processed batch file, then a error
 * "Label not found" is displayed and the batch file is aborted. Any parent
 * batches are ALSO aborted. Same behavior if called without any parameter.
 *
 * if called outside of a batch file, this command has no effect (and does not
 * display any error, even when called without any argument).
 *
 * it reacts to /? by outputing its help screen
 *
 * only 1st argument is processed, any extra arguments are ignored (even /?).
 *
 * Labels can be written as:
 * :LABEL
 *    :LABEL
 *   :  LABEL
 *
 * Labels are searched in the batch file from top to bottom and first match
 * is jumped to.
 */

static enum cmd_result cmd_goto(struct cmd_funcparam *p) {

  /* help? reacts only to /? being passed as FIRST argument */
  if ((p->argc > 0) && imatch(p->argv[0], "/?")) {
    outputnl("Directs batch processing to a labelled line in the batch program.");
    outputnl("");
    outputnl("GOTO LABEL");
    outputnl("");
    outputnl("LABEL specifies a text string used in the batch program as a label.");
    outputnl("");
    outputnl("A label is on a line by itself and must be preceded by a colon.");
    return(CMD_OK);
  }

  /* not inside a batch file? then do nothing */
  if (p->rmod->bat == NULL) return(CMD_OK);

  /* TODO open batch file and read it line by line until label is found or EOF */

  /* TODO if label not found, display error message and abort all batch scripts */

  return(CMD_OK);
}

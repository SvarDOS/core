/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2024 Mateusz Viste
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
 * del/erase
 */

static enum cmd_result cmd_del(struct cmd_funcparam *p) {
  const char *delspec = NULL;
  unsigned short err = 0;
  unsigned short confirmflag = 0;
  unsigned short i;
  unsigned short pathlimit = 0;
  char *buff = p->BUFFER;

  struct DTA *dta = (void *)0x80; /* use the default DTA at location 80h in PSP */
  char *fname = dta->fname;

  if (cmd_ishlp(p)) {
    nls_outputnl(36,0); /* "Deletes one or more files." */
    outputnl("");
    nls_outputnl(36,1); /* "DEL [drive:][path]filename [/P]" */
    nls_outputnl(36,2); /* "ERASE [drive:][path]filename [/P]" */
    outputnl("");
    nls_outputnl(36,3); /* "[drive:][path]filename  Specifies the file(s) to delete." */
    nls_outputnl(36,4); /* "/P  Prompts for confirmation before deleting each file." */
    return(CMD_OK);
  }

  if (p->argc == 0) {
    nls_outputnl(0,7); /* "Required parameter missing" */
    return(CMD_FAIL);
  }

  /* scan argv for delspec and possible /p or /v */
  for (i = 0; i < p->argc; i++) {
    /* delspec? */
    if (p->argv[i][0] == '/') {
      if (imatch(p->argv[i], "/p")) {
        confirmflag = 1;
      } else {
        nls_output(0,2); /* "Invalid switch" */
        output(": ");
        outputnl(p->argv[i]);
        return(CMD_FAIL);
      }
    } else if (delspec != NULL) { /* otherwise its a delspec */
      nls_outputnl(0,4); /* "Too many parameters" */
      return(CMD_FAIL);
    } else {
      delspec = p->argv[i];
    }
  }

  /* convert path to canonical form */
  file_truename(delspec, buff);

  /* is delspec pointing at a directory? if so, add a \*.* */
  i = path_appendbkslash_if_dir(buff);
  if (buff[i - 1] == '\\') sv_strcat(buff, "????????.???");

  /* parse delspec in buff and remember where last backslash or slash is */
  for (i = 0; buff[i] != 0; i++) if (buff[i] == '\\') pathlimit = i + 1;

  /* is this about deleting all content inside a directory? if no per-file
   * confirmation set, ask for a global confirmation */
  if ((confirmflag == 0) && (imatch(buff + pathlimit, "????????.???"))) {
    nls_outputnl(36,5); /* "All files in directory will be deleted!" */
    if (askchoice(svarlang_str(36,6)/*"Are you sure?"*/, svarlang_str(0,10)/*"YN"*/) != 0) return(CMD_FAIL);
  }

  for (i = 0;; i = 1) {

    /* exec FindFirst or FindNext */
    if (i == 0) {
      err = findfirst(dta, buff, DOS_ATTR_RO | DOS_ATTR_SYS | DOS_ATTR_HID);
      if (err != 0) { /* report the error only if query had no wildcards */
        for (i = 0; buff[i] != 0; i++) if (buff[i] == '?') break;
        if (buff[i] == 0) nls_outputnl_doserr(err);
        break;
      }
    } else {
      if (findnext(dta) != 0) break; /* do not report errors on findnext() */
    }

    /* prep the full path/name of the file in buff */
    /* NOTE: buff contained the search pattern but it is no longer needed so I
     * can reuse it now */
    sv_strcpy(buff + pathlimit, fname);

    /* ask if confirmation required: PLIK.TXT  Delete (Y/N)? */
    if (confirmflag) {
      output(buff);
      output(" \t");
      if (askchoice(svarlang_str(36,7)/*"Delete?"*/, svarlang_str(0,10)) != 0) continue;
    }

    /* del found file */
    _asm {
      push ax
      push dx
      mov ah, 0x41      /* delete a file, DS:DX points to an ASCIIZ filespec (no wildcards allowed) */
      mov dx, buff
      int 0x21
      jnc DONE
      mov [err], ax
      DONE:
      pop dx
      pop ax
    }

    if (err != 0) {
      output(fname);
      output(": ");
      nls_outputnl_doserr(err);
      break;
    }
  }

  if (err == 0) return(CMD_OK);
  return(CMD_FAIL);
}

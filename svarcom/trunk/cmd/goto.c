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
 * A label can also be followed by one or more space or tabs, followed by
 * anything. The label is parsed only to the first space, tab, end of line or
 * end of file. Hence this would be perfectly ok:
 *
 * :LABEL this is a comment
 *
 * Labels are searched in the batch file from top to bottom and first match
 * is jumped to. Matching labels is case-insensitive (ie. LABEL == LaBeL)
 */


static void goto_close_dos_handle(unsigned short fhandle) {
  _asm {
    push bx

    mov ah, 0x3e
    mov bx, fhandle
    int 0x21

    pop bx
  }
}


static enum cmd_result cmd_goto(struct cmd_funcparam *p) {
  char *buff = NULL;
  const char *label;
  unsigned short bufflen = 0;
  unsigned short fhandle = 0;
  unsigned short doserr = 0;
  unsigned short batname_seg;
  unsigned short batname_off;
  unsigned char eof_reached = 0;
  unsigned short i;

  /* help? reacts only to /? being passed as FIRST argument */
  if ((p->argc > 0) && imatch(p->argv[0], "/?")) {
    nls_outputnl(17,0); /* "Directs batch processing to a labelled line in the batch program." */
    outputnl("");
    nls_outputnl(17,1); /* "GOTO LABEL" */
    outputnl("");
    nls_outputnl(17,2); /* "LABEL specifies a text string used in the batch program as a label." */
    outputnl("");
    nls_outputnl(17,3); /* "A label is on a line by itself and must be preceded by a colon." */
    return(CMD_OK);
  }

  /* not inside a batch file? not given any argument? then do nothing */
  if ((p->rmod->bat == NULL) || (p->argc == 0)) return(CMD_OK);

  /* label is in first arg */
  label = p->argv[0];

  /* open batch file (read-only) */
  batname_seg = FP_SEG(p->rmod->bat->fname);
  batname_off = FP_OFF(p->rmod->bat->fname);
  _asm {
    push bx
    push dx

    mov ax, batname_seg
    push ds /* save ds */
    mov ds, ax
    mov dx, batname_off
    mov ax, 0x3d00
    int 0x21    /* handle in ax on success */
    pop ds
    jnc OPEN_SUCCESS
    mov doserr, ax

    OPEN_SUCCESS:
    mov fhandle, ax /* save file handle */

    pop dx
    pop bx
  }

  /* file open failed? */
  if (doserr != 0) {
    nls_outputnl_doserr(doserr);
    return(CMD_FAIL);
  }

  /* reset the rmod bat counter since I will scan all lines from top to bottom */
  p->rmod->bat->nextline = 0; /* remember this is a byte offset, not a line number */

  /* read bat file line by line until label is found or EOF */
  for (;;) {

    /* move any existing data to the start of the buffer */
    if (bufflen > 0) memmove(p->BUFFER, buff, bufflen);

    buff = p->BUFFER; /* assumption: must be big enough to hold 2 sectors (2 * 512) */

    /* if buffer has less than 512b then load it with another sector (unless eof) */
    if ((eof_reached == 0) && (bufflen < 512)) {
      /* load 512b of data into buffer */
      _asm {
        push ax
        push bx
        push cx
        push dx
        pushf

        mov ah, 0x3f    /* read from file handle */
        mov bx, fhandle /* file handle where to read from */
        mov cx, 512     /* read 512 bytes (one sector) */
        mov dx, buff    /* target buffer */
        add dx, bufflen /* data must follow existing pending data */
        int 0x21        /* CF clear on success and AX=number of bytes read */
        /* error? */
        jnc READ_OK
        mov doserr, ax
        READ_OK:
        add bufflen, ax
        /* set eof if amount of bytes read is shorter than cx */
        cmp ax, cx
        je EOF_NOT_REACHED
        mov eof_reached, byte ptr 1
        EOF_NOT_REACHED:

        popf
        pop dx
        pop cx
        pop bx
        pop ax
      }

      /* on error close the file and quit */
      if (doserr != 0) {
        goto_close_dos_handle(fhandle);
        nls_outputnl_doserr(doserr);
        return(CMD_FAIL);
      }
    }

    /* advance buffer to first non-space/non-tab/non-CR/non-LF */
    while (bufflen > 0) {
      if ((*buff != ' ') && (*buff != '\t') && (*buff != '\r') && (*buff != '\n')) break;
      bufflen--;
      buff++;
      p->rmod->bat->nextline++;
    }

    /* if the line does not start with a colon, then jump to next line */
    if ((bufflen > 0) && (*buff != ':')) {
      while ((bufflen > 0) && (*buff != '\n')) {
        bufflen--;
        buff++;
        p->rmod->bat->nextline++;
      }
    }

    /* refill buffer if needed */
    if ((bufflen < 512) && (eof_reached == 0)) continue;

    /* eof? */
    if (bufflen == 0) break;

    /* skip the colon */
    if (*buff == ':') {
      bufflen--;
      buff++;
      p->rmod->bat->nextline++;
    }

    /* skip any leading white spaces (space or tab) */
    while (bufflen > 0) {
      if ((*buff != ' ') && (*buff != '\t')) break;
      bufflen--;
      buff++;
      p->rmod->bat->nextline++;
    }

    /* read the in-file label and compare it with what's in the label buff */
    for (i = 0;; i++) {
      /* if end of label then check if it is also end of in-file label (ends with space, tab, \r or \n) */
      if ((i == bufflen) || (buff[i] == ' ') || (buff[i] == '\t') || (buff[i] == '\r') || (buff[i] == '\n')) {
        if (label[i] == 0) {
          /* match found -> close file, skip to end of line and quit */
          while (bufflen > 0) {
            bufflen--;
            buff++;
            p->rmod->bat->nextline++;
            if (*buff == '\n') break;
          }
          goto_close_dos_handle(fhandle);
          return(CMD_OK);
        }
        break;
      }
      /* end of label = mismatch */
      if (label[i] == 0) break;
      /* case-insensitive comparison */
      if ((label[i] & 0xDF) != (buff[i] & 0xDF)) break;
    }

    /* no match, move forward to end of line and repeat */
    while ((bufflen > 0) && (*buff != '\n')) {
      bufflen--;
      buff++;
      p->rmod->bat->nextline++;
    }
  }

  /* close the batch file handle */
  goto_close_dos_handle(fhandle);

  /* label not found, display error message and abort all batch scripts */
  nls_outputnl(17, 10); /* "Label not found" */
  rmod_free_bat_llist(p->rmod);

  /* restore echo flag as it was before running the (first) bat file */
  p->rmod->flags &= ~FLAG_ECHOFLAG;
  if (p->rmod->flags & FLAG_ECHO_BEFORE_BAT) p->rmod->flags |= FLAG_ECHOFLAG;

  return(CMD_FAIL);
}

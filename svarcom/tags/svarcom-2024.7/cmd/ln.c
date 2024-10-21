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
 * ln add linkname linkdir
 * ln del linkname
 * ln list [pattern]
 */

static enum cmd_result cmd_lnadd(char *BUFFER, const char *linkname, const char *targetdir, unsigned short env_seg) {
  const char *ext;
  char *realdirname = BUFFER;
  unsigned short realdirnamelen;
  char *buff = BUFFER + 256;
  unsigned short doserr;

  /* convert dirname to realpath */
  doserr = file_truename(targetdir, realdirname);
  if (doserr != 0) {
    nls_outputnl_doserr(doserr);
    return(CMD_FAIL);
  }

  /* does EXENAME in DIRECTORY exist? */
  if (lookup_cmd(buff, linkname, realdirname, &ext) != 0) {
    nls_outputnl(29,4); /* "No matching executable found in given path." */
    return(CMD_FAIL);
  }

  /* compute the filename of the link file */
  if (link_computefname(buff, linkname, env_seg) != 0) return(CMD_FAIL);

  realdirnamelen = strlen(realdirname);

  /* open file *only if it does not exist yet* and write realdirname to it */
  _asm {
    push ax
    push bx
    push cx
    push dx

    mov ax, 0x6c00  /* extended OPEN */
    mov bx, 1       /* open for WRITE */
    xor cx, cx      /* create the file with no attributes */
    mov dx, 0x0010  /* create file if it does not exists, otherwise fail */
    mov si, buff    /* file name */
    int 0x21
    jnc WRITE
    mov doserr, ax
    jmp DONE

    WRITE:
    mov bx, ax      /* file handle */
    mov ah, 0x40    /* write to file */
    mov cx, realdirnamelen
    mov dx, realdirname
    int 0x21

    mov ah, 0x3e    /* close file in BX */
    int 0x21

    DONE:

    pop dx
    pop cx
    pop bx
    pop ax
  }

  if (doserr != 0) {
    nls_outputnl_doserr(doserr);
    return(CMD_FAIL);
  }

  return(CMD_OK);
}


static enum cmd_result cmd_lndel(char *buff, const char *linkname, unsigned short env_seg) {
  unsigned short i;

  /* is the argument valid? (must not contain any dot nor backslash) */
  for (i = 0; linkname[i] != 0; i++) {
    if ((linkname[i] == '.') || (linkname[i] == '/') || (linkname[i] == '\\')) {
      nls_outputnl(0,3); /* "Invalid parameter format" */
      return(CMD_OK);
    }
  }

  /* prep link filename to look at */
  if (link_computefname(buff, linkname, env_seg) != 0) return(CMD_FAIL);

  /* try removing it */
  i = 0;
  _asm {
    push ax
    push bx
    push cx
    push dx

    mov ah, 0x41
    mov dx, buff
    int 0x21
    jnc SUCCESS
    mov i, ax
    SUCCESS:

    pop dx
    pop cx
    pop bx
    pop ax
  }

  if (i != 0) nls_outputnl_doserr(i);

  return(CMD_OK);
}


static enum cmd_result cmd_lnlist(char *buff, const char *linkname, unsigned short env_seg) {
  unsigned short i, pathlen;
  struct DTA *dta = (void *)0x80;
  char *buff128 = buff + 256;
  char *buff16 = buff128 + 128;

  if (linkname != NULL) {
    /* make sure link pattern is valid (must not contain '.' or '\\' or '/') */
    for (i = 0; linkname[i] != 0; i++) {
      switch (linkname[i]) {
        case '.':
        case '/':
        case '\\':
          nls_outputnl(0,3); /* "Invalid parameter format" */
          return(CMD_FAIL);
      }
    }
  } else {
    linkname = "*";
  }

  /* prep DOSDIR\LINKS\pattern */
  if (link_computefname(buff, linkname, env_seg) != 0) return(CMD_FAIL);

  /* set pathlen to end of path (char after last backslash) */
  pathlen = 0;
  for (i = 0; buff[i] != 0; i++) if (buff[i] == '\\') pathlen = i + 1;

  if (findfirst(dta, buff, DOS_ATTR_RO | DOS_ATTR_ARC) != 0) return(CMD_OK);

  do {
    /* print link file name (but trim ".lnk") */
    for (i = 0; (dta->fname[i] != 0) && (dta->fname[i] != '.'); i++) buff16[i] = dta->fname[i];
    if (i < 8) buff16[i++] = '\t';
    buff16[i] = 0;
    output(buff16);
    output(" @ ");
    /* prep full link filename */
    strcpy(buff + pathlen, dta->fname);
    /* read up to 128 bytes from link file to buff and display it */
    i = 0;
    _asm {
      push ax
      push bx
      push cx
      push dx

      /* open file */
      mov ax, 0x3d00
      mov dx, buff   /* filename */
      int 0x21
      jc FAIL_FOPEN
      /* read from file */
      mov bx, ax
      mov ah, 0x3f
      mov cx, 128
      mov dx, buff128
      int 0x21
      jc FAIL_FREAD
      mov i, ax
      /* close file */
      FAIL_FREAD:
      mov ah, 0x3e
      int 0x21
      FAIL_FOPEN:

      pop dx
      pop cx
      pop bx
      pop ax
    }
    buff128[i] = 0;
    /* make sure no cr or lf is present */
    for (i = 0; buff128[i] != 0; i++) {
      if ((buff128[i] == '\r') || (buff128[i] == '\n')) {
        buff128[i] = 0;
        break;
      }
    }
    outputnl(buff128);
  } while (findnext(dta) == 0);

  return(CMD_OK);
}


static enum cmd_result cmd_ln(struct cmd_funcparam *p) {
  if (cmd_ishlp(p)) {
    nls_outputnl(29,0); /* "Adds, deletes or displays executable links." */
    outputnl("");
    nls_outputnl(29,1); /* "LN ADD linkname targetdir" */
    nls_outputnl(29,2); /* "LN DEL linkname" */
    nls_outputnl(29,3); /* "LN LIST [pattern]" */
    return(CMD_OK);
  }

  if (p->argc == 0) {
    nls_outputnl(0,7); /* "Required parameter missing */
    return(CMD_OK);
  }

  if (env_lookup(p->env_seg, "DOSDIR") == NULL) {
    nls_outputnl(29,5); /* "%DOSDIR% not defined" */
    return(CMD_FAIL);
  }

  /* detect what subfunction the user wants */
  if ((imatch(p->argv[0], "add")) && (p->argc == 3)) return(cmd_lnadd(p->BUFFER, p->argv[1], p->argv[2], p->env_seg));
  if ((imatch(p->argv[0], "del")) && (p->argc == 2)) return(cmd_lndel(p->BUFFER, p->argv[1], p->env_seg));
  if (imatch(p->argv[0], "list")) {
    if (p->argc == 1) return(cmd_lnlist(p->BUFFER, NULL, p->env_seg));
    if (p->argc == 2) return(cmd_lnlist(p->BUFFER, p->argv[1], p->env_seg));
  }

  nls_outputnl(0,6); /* "Invalid parameter" */
  return(CMD_FAIL);
}

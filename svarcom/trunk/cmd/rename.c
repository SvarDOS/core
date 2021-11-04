/*
 * rename/ren
 */

static int cmd_rename(struct cmd_funcparam *p) {
  char *src = p->BUFFER;
  char *dst = p->BUFFER + (BUFFER_SIZE / 4);
  char *buff1 = p->BUFFER + (BUFFER_SIZE / 4 * 2);
  char *buff2 = p->BUFFER + (BUFFER_SIZE / 4 * 3);
  unsigned short i, fnameoffset;
  struct DTA *dta = (void *)0x80; /* use default DTA in PSP */

  if (cmd_ishlp(p)) {
    outputnl("Renames a file or files");
    outputnl("");
    outputnl("RENAME [drive:][path]filename1 filename2");
    outputnl("REN [drive:][path]filename1 filename2");
    outputnl("");
    outputnl("Note that you cannot specify a new drive or path for your destination file.");
    outputnl("Use MOVE to rename a directory, or to move files from one directory to another.");
    return(-1);
  }

  /* I expect exactly two arguments */
  if (p->argc != 2) {
    outputnl("Invalid syntax");
    return(-1);
  }

  /* convert src to truename format */
  i = file_truename(p->argv[0], src);
  if (i != 0) {
    outputnl(doserr(i));
    return(-1);
  }

  /* copy src path to buffers and remember where the filename starts */
  fnameoffset = 0;
  for (i = 0;; i++) {
    buff1[i] = src[i];
    buff2[i] = src[i];
    if (buff1[i] == '\\') fnameoffset = i + 1;
    if (buff1[i] == 0) break;
  }

  /* now append dst filename to the buffer and validate it: cannot contain backslash, slash or ':' */
  for (i = 0;; i++) {
    switch (p->argv[1][i]) {
      case ':':
      case '\\':
      case '/':
        outputnl("Invalid destination");
        return(-1);
    }
    buff1[fnameoffset + i] = p->argv[1][i];
    if (buff1[fnameoffset + i] == 0) break;
  }

  /* apply truename to dest to normalize wildcards into ? chars */
  i = file_truename(buff1, dst);
  if (i != 0) {
    outputnl(doserr(i));
    return(-1);
  }

  /* we're good to go, src and dst should look somehow like that now:
   * src   =   C:\TEMP\PATH\FILE????.TXT
   * dst   =   C:\TEMP\PATH\FILE????.DOC
   * buff1 =   C:\TEMP\PATH\
   * buff2 =   C:\TEMP\PATH\
   * fnameoffset = 13
   *
   * src is used for FindFirst/FindNext iterations, then buff1 is filled with
   * the source filename found by FindFirst/FindNext and buff2 is filled with
   * the destination file (with ?'s replaced by whatever is found at the same
   * location in buff1).
   */

  i = findfirst(dta, src, 0);
  if (i != 0) outputnl(doserr(i));

  while (i == 0) {
    /* write found fname into buff1 and dst fname into buff2 - both in FCB
     * format (MYFILE  EXT) so it is easy to compare them */
    file_fname2fcb(buff1 + fnameoffset, dta->fname);
    file_fname2fcb(buff2 + fnameoffset, dst + fnameoffset);

    /* scan buff2 fname for '?' and replace them with whatever is in buff1 */
    for (i = fnameoffset; buff2[i] != 0; i++) {
      if (buff2[i] == '?') buff2[i] = buff1[i];
    }

    /* fill buff1 with the 8+3 found file and convert the one in buff2 to 8+3 as well */
    file_fcb2fname(buff1 + fnameoffset, buff2 + fnameoffset);
    strcpy(buff2 + fnameoffset, buff1 + fnameoffset);
    strcpy(buff1 + fnameoffset, dta->fname);

    /* buff1 contains now a fully resolved source and buff2 a proper destination */
    #if 0  /* DEBUG ("if 1" to enable) */
    output(buff1);
    output(" -> ");
    outputnl(buff2);
    #endif
    /* call DOS to do the actual job */
    i = 0;
    _asm {
      push ax
      push di
      push dx
      push es

      mov ah, 0x56  /* rename file: DS:DX=ASCIIZ of src  ES:DI=ASCIIZ of dst */
      push ds
      pop es
      mov dx, buff1
      mov di, buff2
      int 0x21      /* CF clear on success, otherwise err code in AX */
      jnc DONE
      mov [i], ax   /* copy error code to i */
      DONE:

      pop es
      pop dx
      pop di
      pop ax
    }
    if (i != 0) {
      output(buff1 + fnameoffset);
      output(" -> ");
      output(buff2 + fnameoffset);
      output("  ");
      outputnl(doserr(i));
    }
    /* next please */
    i = findnext(dta);
  }

  return(-1);
}

/*
 * copy
 */

/* /A - Used to copy ASCII files. Applies to the filename preceding it and to
 * all following filenames. Files will be copied until an end-of-file mark is
 * encountered in the file being copied. If an end-of-file mark is encountered
 * in the file, the rest of the file is not copied. DOS will append an EOF
 * mark at the end of the copied file.
 *
 * /B - Used to copy binary files. Applies to the filename preceding it and to
 * all following filenames. Copied files will be read by size (according to
 * the number of bytes indicated in the file`s directory listing). An EOF mark
 * is not placed at the end of the copied file.
 *
 * /V - Checks after the copy to assure that a file was copied correctly. If
 * the copy cannot be verified, the program will display an error message.
 * Using this option will result in a slower copying process.
 *
 * special case: "COPY A+B+C+D" means "append B, C and D files to the A file"
 * if A does not exist, then "append C and D to B", etc.
 */

struct copy_setup {
  const char *src[64];
  unsigned short src_count; /* how many sources are declared */
  char dst[256];
  unsigned short dstlen;
  char src_asciimode[64];
  char dst_asciimode;
  char last_asciimode; /* /A or /B impacts the file preceding it and becomes the new default for all files that follow */
  char verifyflag;
  char lastitemwasplus;
  char databuf[BUFFER_SIZE - 1024];
};


/* appends a backslash if path is a directory
 * returns the (possibly updated) length of path */
static unsigned short cmd_copy_addbkslash_if_dir(char *path) {
  unsigned short len;
  int attr;
  for (len = 0; path[len] != 0; len++);
  if (len == 0) return(0);
  if (path[len - 1] == '\\') return(len);
  /* */
  attr = file_getattr(path);
  if ((attr > 0) && (attr & DOS_ATTR_DIR)) {
    path[len++] = '\\';
    path[len] = 0;
  }
  return(len);
}


/* copies src to dst, overwriting or appending to the destination.
 * - copy is performed in ASCII mode if asciiflag set (stop at first EOF in src
 *   and append an EOF in dst).
 * - returns zero on success, DOS error code on error */
unsigned short cmd_copy_internal(const char *dst, char dstascii, const char *src, char srcascii, unsigned char appendflag, void *buff, unsigned short buffsz) {
  unsigned short errcode = 0;
  unsigned short srch = 0xffff, dsth = 0xffff;
  _asm {

    /* open src */
    OPENSRC:
    mov ax, 0x3d00 /* DOS 2+ -- open an existing file, read access mode */
    mov dx, src    /* ASCIIZ fname */
    int 0x21       /* CF clear on success, handle in AX */
    mov [srch], ax /* store src handle in memory */

    /* check appendflag so I know if I have to try opening dst for append */
    xor al, al
    or al, [appendflag]
    jz CREATEDST

    /* try opening dst first if appendflag set */
    mov ax, 0x3d01 /* DOS 2+ -- open an existing file, write access mode */
    mov dx, dst    /* ASCIIZ fname */
    int 0x21       /* CF clear on success, handle in AX */
    jc CREATEDST   /* failed to open file (file does not exist) */
    mov [dsth], ax /* store dst handle in memory */

    /* got file open, LSEEK to end of it now so future data is appended */
    mov bx, ax     /* file handle in BX (was still in AX) */
    mov ax, 0x4202 /* DOS 2+ -- set file pointer to end of file + CX:DX */
    xor cx, cx     /* offset zero */
    xor dx, dx     /* offset zero */
    int 0x21       /* CF set on error */
    jc FAIL
    jmp COPY

    /* create dst */
    CREATEDST:
    mov ah, 0x3c   /* DOS 2+ -- create a file */
    mov dx, dst
    xor cx, cx     /* zero out attributes */
    int 0x21       /* handle in AX on success, CF set on error */
    jc FAIL
    mov [dsth], ax /* store dst handle in memory */

    /* perform actual copy */
    COPY:
    /* read a block from src */
    mov ah, 0x3f   /* DOS 2+ -- read from file */
    mov bx, [srch]
    mov cx, [buffsz]
    mov dx, [buff] /* DX points to buffer */
    int 0x21       /* CF set on error, bytes read in AX (0=EOF) */
    jc FAIL        /* abort on error */
    /* EOF? (ax == 0) */
    test ax, ax
    jz ENDOFFILE
    /* write block of AX bytes to dst */
    mov cx, ax     /* block length */
    mov ah, 0x40   /* DOS 2+ -- write to file (CX bytes from DS:DX) */
    mov bx, [dsth] /* file handle */
    /* mov dx, [buff] */ /* DX points to buffer already */
    int 0x21       /* CF clear and AX=CX on success */
    jc FAIL
    cmp ax, cx     /* sould be equal, otherwise failed */
    mov ax, 0x08   /* preset to DOS error "Insufficient memory" */
    jne FAIL
    jmp COPY

    ENDOFFILE:
    /* if dst ascii mode -> add an EOF (ASCII mode not supported for the time being) */

    jmp CLOSESRC

    FAIL:
    mov [errcode], ax

    CLOSESRC:
    /* close src and dst */
    mov bx, [srch]
    cmp bx, 0xffff
    je CLOSEDST
    mov ah, 0x3e   /* DOS 2+ -- close a file handle */
    int 0x21

    CLOSEDST:
    mov bx, [dsth]
    cmp bx, 0xffff
    je DONE
    mov ah, 0x3e   /* DOS 2+ -- close a file handle */
    int 0x21

    DONE:
  }
  return(errcode);
}


static int cmd_copy(struct cmd_funcparam *p) {
  struct copy_setup *setup = (void *)(p->BUFFER);
  unsigned short i;
  unsigned short copiedcount_in = 0, copiedcount_out = 0; /* number of input/output copied files */
  struct DTA *dta = (void *)0x80; /* use DTA at default location in PSP */

  if (cmd_ishlp(p)) {
    outputnl("Copies one or more files to another location.");
    outputnl("");
    outputnl("COPY [/A|/B] source [/A|/B] [+source [/A|/B] [+...]] [destination [/A|/B]] [/V]");
    outputnl("");
    outputnl("source       Specifies the file or files to be copied");
    outputnl("/A           Indicates an ASCII text file");
    outputnl("/B           Indicates a binary file");
    outputnl("destination  Specifies the directory and/or filename for the new file(s)");
    outputnl("/V           Verifies that new files are written correctly");
    outputnl("");
    outputnl("To append files, specify a single file for destination, but multiple files");
    outputnl("for source (using wildcards or file1+file2+file3 format).");
    outputnl("");
    outputnl("NOTE: /A and /B are no-ops (ignored), provided only for compatibility reasons.");
    return(-1);
  }

  /* parse cmdline and fill the setup struct accordingly */

  memset(setup, 0, sizeof(*setup));

  for (i = 0; i < p->argc; i++) {

    /* switch? */
    if (p->argv[i][0] == '/') {
      if ((imatch(p->argv[i], "/a")) || (imatch(p->argv[i], "/b"))) {
        setup->last_asciimode = 'b';
        if (imatch(p->argv[i], "/a")) setup->last_asciimode = 'a';
        /* */
        if (setup->dst[0] != 0) {
          setup->dst_asciimode = setup->last_asciimode;
        } else if (setup->src_count != 0) {
          setup->src_asciimode[setup->src_count - 1] = setup->last_asciimode;
        }
      } else if (imatch(p->argv[i], "/v")) {
        setup->verifyflag = 1;
      } else {
        outputnl("Invalid switch");
        return(-1);
      }
      continue;
    }

    /* not a switch - must be either a source, a destination or a + */
    if (p->argv[i][0] == '+') {
      /* a plus cannot appear after destination or before first source */
      if ((setup->dst[0] != 0) || (setup->src_count == 0)) {
        outputnl("Invalid syntax");
        return(-1);
      }
      setup->lastitemwasplus = 1;
      /* a plus may be immediately followed by a filename - if so, emulate
       * a new argument */
      if (p->argv[i][1] != 0) {
        p->argv[i] += 1;
        i--;
      }
      continue;
    }

    /* src? (first non-switch or something that follows a +) */
    if ((setup->lastitemwasplus) || (setup->src_count == 0)) {
      setup->src[setup->src_count] = p->argv[i];
      setup->src_asciimode[setup->src_count] = setup->last_asciimode;
      setup->src_count++;
      setup->lastitemwasplus = 0;
      continue;
    }

    /* must be a dst then */
    if (setup->dst[0] != 0) {
      outputnl("Invalid syntax");
      return(-1);
    }
    if (file_truename(p->argv[i], setup->dst) != 0) {
      outputnl("Invalid destination");
      return(-1);
    }
    setup->dst_asciimode = setup->last_asciimode;
    /* if dst is a directory then append a backslash */
    setup->dstlen = cmd_copy_addbkslash_if_dir(setup->dst);
  }

  /* DEBUG: output setup content ("if 1" to enable) */
  #if 1
  printf("src: ");
  for (i = 0; i < setup->src_count; i++) {
    if (i != 0) printf(", ");
    printf("%s [%c]", setup->src[i], setup->src_asciimode[i]);
  }
  printf("\r\n");
  printf("dst: %s [%c]\r\n", setup->dst, setup->dst_asciimode);
  printf("verify: %s\r\n", (setup->verifyflag)?"ON":"OFF");
  #endif

  /* must have at least one source */
  if (setup->src_count == 0) {
    outputnl("Required parameter missing");
    return(-1);
  }

  /* perform the operation based on setup directives:
   * iterate over every source and copy it to dest */

  for (i = 0; i < setup->src_count; i++) {
    unsigned short t;
    unsigned short databuflen;
    unsigned short pathendoffset;

    /* resolve truename of src and write it to buffer */
    t = file_truename(setup->src[i], setup->databuf);
    if (t != 0) {
      output(setup->src[i]);
      output(" - ");
      outputnl(doserr(t));
      continue;
    }
    databuflen = strlen(setup->databuf); /* remember databuf length */

    /* if length zero, skip (not sure why this would be possible, though) */
    if (databuflen == 0) continue;

    /* if src does not end with a backslash AND it is a directory then append a backslash */
    databuflen = cmd_copy_addbkslash_if_dir(setup->databuf);

    /* if src ends with a '\' then append *.* */
    if (setup->databuf[databuflen - 1] == '\\') {
      strcat(setup->databuf, "*.*");
    }

    /* remember where the path in databuf ends */
    for (t = 0; setup->databuf[t] != 0; t++) {
      if (setup->databuf[t] == '\\') pathendoffset = t + 1;
    }

    /* */
    if (findfirst(dta, setup->databuf, 0) != 0) {
      continue;
    }

    do {
      char appendflag;
      if (dta->attr & DOS_ATTR_DIR) continue; /* skip directories */

      /* compute full path/name of the file */
      strcpy(setup->databuf + pathendoffset, dta->fname);

      /* if there was no destination, then YOU are the destination now!
       * this handles situations like COPY a.txt+b.txt+c.txt */
      if (setup->dst[0] == NULL) {
        strcpy(setup->dst, setup->databuf);
        setup->dstlen = strlen(setup->dst);
        copiedcount_in++;
        copiedcount_out++;
        continue;
      }

      /* is dst ending with a backslash? then append fname to it */
      if (setup->dst[setup->dstlen - 1] == '\\') strcpy(setup->dst + setup->dstlen, dta->fname);

      /* now databuf contains the full source and dst contains the full dest... COPY TIME! */

      /* if dst file exists already -> overwrite it or append?
          - if dst is a dir (dstlen-1 points at a \\) -> overwrite
          - otherwise: if copiedcount_in==0 overwrite, else append */
      output(setup->databuf);
      if ((setup->dst[setup->dstlen - 1] == '\\') || (copiedcount_in == 0)) {
        appendflag = 0;
        output(" > ");
        copiedcount_out++;
      } else {
        appendflag = 1;
        output(" >> ");
      }
      outputnl(setup->dst);

      t = cmd_copy_internal(setup->dst, 0, setup->databuf, 0, appendflag, setup->databuf, sizeof(setup->databuf));
      if (t != 0) {
        outputnl(doserr(t));
        return(-1);
      }

      copiedcount_in++;
    } while (findnext(dta) == 0);

  }

  sprintf(setup->databuf, "%u file(s) copied", copiedcount_out);
  outputnl(setup->databuf);

  return(-1);
}

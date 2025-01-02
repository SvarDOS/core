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
  char cursrc[256];         /* buffer for currently processed src */
  char dst[256];
  unsigned short dstlen;
  char src_asciimode[64];
  char dst_asciimode;
  char last_asciimode; /* /A or /B impacts the file preceding it and becomes the new default for all files that follow */
  char verifyflag;
  char lastitemwasplus;
  unsigned short databufsz;
  unsigned short databufseg_custom; /* seg of buffer if dynamically allocated (0 otherwise) */
  char databuf[1];
};


static void dos_freememseg(unsigned short seg);
#pragma aux dos_freememseg = \
"mov ah, 0x49" \
"int 0x21" \
modify [ax] \
parm [es]


/* returns 0 on success */
static unsigned short dos_delfile(const char *f);
#pragma aux dos_delfile = \
"mov ah, 0x41" \
"xor cx, cx" \
"int 0x21" \
"jc DONE" \
"xor ax, ax" \
"DONE:" \
modify [cx] \
parm [dx] \
value [ax]


/* copies src to dst, overwriting or appending to the destination.
 * - copy is performed in ASCII mode if asciiflag set (stop at first EOF in src
 *   and append an EOF in dst).
 * - returns zero on success, DOS error code on error */
static unsigned short cmd_copy_internal(const char *dst, char dstascii, const char *src, char srcascii, unsigned char appendflag, void far *buff, unsigned short buffsz) {
  unsigned short errcode = 0;
  unsigned short srch = 0xffff, dsth = 0xffff;
  unsigned short buffseg = FP_SEG(buff);
  unsigned short buffoff =  FP_OFF(buff);
  (void)dstascii; (void)srcascii;
  _asm {
    push ax
    push bx
    push cx
    push dx
    push ds

    /* open src */
    OPENSRC:
    mov ax, 0x3d00 /* DOS 2+ -- open an existing file, read access mode */
    mov dx, src    /* ASCIIZ fname */
    int 0x21       /* CF clear on success, handle in AX */
    jc FAIL
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
    jmp short COPY

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
    mov ds, buffseg
    COPY_LOOP:
    /* read a block from src */
    mov ah, 0x3f   /* DOS 2+ -- read from file */
    mov bx, [srch]
    mov cx, [buffsz]
    mov dx, [buffoff] /* DX points to buffer */
    int 0x21       /* CF set on error, bytes read in AX (0=EOF) */
    jc FAIL        /* abort on error */
    /* EOF? (ax == 0) */
    test ax, ax
    jz ENDOFFILE
    /* write block of AX bytes to dst */
    mov cx, ax     /* block length */
    mov ah, 0x40   /* DOS 2+ -- write to file (CX bytes from DS:DX) */
    mov bx, [dsth] /* file handle */
    /* mov dx, buffoff */ /* DX points to buffer already */
    int 0x21       /* CF clear and AX=CX on success */
    jc FAIL
    cmp ax, cx     /* should be equal, otherwise failed */
    mov ax, 0x27   /* preset to DOS error "Insufficient disk space" */
    je COPY_LOOP
    jmp short FAIL

    ENDOFFILE:
    /* if dst ascii mode -> add an EOF (ASCII mode not supported for the time being) */

    jmp short CLOSESRC

    FAIL:
    mov [errcode], ax

    /* close src and dst, but first take care to clone the timestamp to dst */
    CLOSESRC:
    mov bx, [srch]
    cmp bx, 0xffff /* skip if not a file */
    je CLOSEDST
    mov ax, 0x5700 /* DOS 2+ - GET FILE'S LAST-WRITTEN DATE AND TIME */
    int 0x21  /* time and date are in CX and DX now */
    /* proceed with closing the file */
    /* mov bx, [srch] */
    mov ah, 0x3e   /* DOS 2+ -- close a file handle */
    int 0x21

    CLOSEDST:
    mov bx, [dsth]
    cmp bx, 0xffff
    je DONE
    /* set timestamp, unless an error occured or operation was appending */
    xor ax, ax
    cmp [errcode], ax
    jne SKIPDATESAVE
    /* skip timesetting also if appending */
    cmp [appendflag], al
    jne SKIPDATESAVE
    /* do the job */
    mov ax, 0x5701 /* DOS 2+ - SET FILE'S LAST-WRITTEN DATE AND TIME */
    /* BX=file handle  CX=TIME  DX=DATE */
    int 0x21
    SKIPDATESAVE:
    mov ah, 0x3e   /* DOS 2+ -- close a file handle */
    int 0x21

    /* remove dst file on error (remember DS still points to databuff) */
    cmp word ptr [errcode], 0
    jz DONE
    mov ah, 0x41
    mov dx, dst
    xor cx, cx
    pop ds
    int 0x21
    push ds

    DONE:

    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
  }
  return(errcode);
}


static enum cmd_result cmd_copy(struct cmd_funcparam *p) {
  struct copy_setup *setup = (void *)(p->BUFFER);
  unsigned short i;
  unsigned short copiedcount_in = 0, copiedcount_out = 0; /* number of input/output copied files */
  struct DTA *dta = crt_temp_dta; /* use default DTA */

  if (cmd_ishlp(p)) {
    nls_outputnl(38,0); /* "Copies one or more files to another location." */
    outputnl("");
    nls_outputnl(38,1); /* "COPY [/A|/B] source [/A|/B] [+source [/A|/B] [+...]] [destination [/A|/B]] [/V]" */
    outputnl("");
    nls_outputnl(38,2); /* "source       Specifies the file or files to be copied" */
    nls_outputnl(38,3); /* "/A           Indicates an ASCII text file" */
    nls_outputnl(38,4); /* "/B           Indicates a binary file" */
    nls_outputnl(38,5); /* "destination  Specifies the directory and/or filename for the new file(s)" */
    nls_outputnl(38,6); /* "/V           Verifies that new files are written correctly" */
    outputnl("");
    nls_outputnl(38,7); /* "To append files, specify a single file for destination, but multiple (...)" */
    outputnl("");
    nls_outputnl(38,8); /* "NOTE: /A and /B are no-ops, provided only for compatibility reasons" */
    return(CMD_OK);
  }

  /* parse cmdline and fill the setup struct accordingly */

  sv_bzero(setup, sizeof(*setup));
  setup->databufsz = (p->BUFFERSZ - sizeof(*setup)) & 0xfe00; /* use a multiple of 512 for better DOS performances */

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
        nls_outputnl(0,2); /* "Invalid switch" */
        return(CMD_FAIL);
      }
      continue;
    }

    /* not a switch - must be either a source, a destination or a + */
    if (p->argv[i][0] == '+') {
      /* a plus cannot appear after destination or before first source */
      if ((setup->dst[0] != 0) || (setup->src_count == 0)) {
        nls_outputnl(0,1); /* "Invalid syntax" */
        return(CMD_FAIL);
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
      nls_outputnl(0,1); /* "Invalid syntax" */
      return(CMD_FAIL);
    }
    if (file_truename(p->argv[i], setup->dst) != 0) {
      nls_outputnl(0,8); /* "Invalid destination" */
      return(CMD_FAIL);
    }
    setup->dst_asciimode = setup->last_asciimode;
    /* if dst is a directory then append a backslash */
    setup->dstlen = path_appendbkslash_if_dir(setup->dst);
  }

  /* DEBUG: output setup content ("if 1" to enable) */
  #if 0
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
    nls_outputnl(0,7); /* "Required parameter missing" */
    return(CMD_FAIL);
  }

  /* alloc as much memory as possible */
  {
    unsigned short alloc_size = 0;
    unsigned short alloc_seg = 0;
    _asm {
      push ax
      push bx
      push dx

      mov ah, 0x48  /* DOS 2+ allocate memory */
      mov bx, 4064  /* number of segments to allocate (4064 segs = 127 * 512 bytes) */
      mov dx, bx    /* my own variable to hold result */
      int 0x21      /* on success AX=segment, on error AX=max number of segments */
      jnc DONE
      TRY_AGAIN:
      /* ask again, this time using the max segments value obtained earlier
       * (but truncated to a multiple of 512 for better copy performances) */
      and ax, 0xffe0 /* it's segments, not bytes (32 segs = 512 bytes) */
      mov bx, ax
      mov dx, bx
      mov ah, 0x48
      int 0x21
      jnc DONE
      xor dx, dx
      DONE:
      /* dx = size ; ax = segment (error if dx = 0) */
      test dx, dx
      jz FINITO
      mov alloc_seg, ax
      mov alloc_size, dx
      FINITO:

      pop dx
      pop bx
      pop ax
    }

    if (alloc_size != 0) {
      /* if dos malloc returned less than the buffer I already have then free it */
      if (alloc_size * 16 < setup->databufsz) {
        dos_freememseg(alloc_seg);
      } else {
        setup->databufseg_custom = alloc_seg;
        setup->databufsz = alloc_size * 16;
      }
    }
  }

  /* perform the operation based on setup directives:
   * iterate over every source and copy it to dest */

  for (i = 0; i < setup->src_count; i++) {
    unsigned short t;
    unsigned short cursrclen;
    unsigned short pathendoffset;

    /* resolve truename of src and write it to buffer */
    t = file_truename(setup->src[i], setup->cursrc);
    if (t != 0) {
      output(setup->src[i]);
      output(" - ");
      nls_outputnl_doserr(t);
      continue;
    }
    cursrclen = sv_strlen(setup->cursrc); /* remember cursrc length */

    /* if length zero, skip (not sure why this would be possible, though) */
    if (cursrclen == 0) continue;

    /* if src does not end with a backslash AND it is a directory then append a backslash */
    cursrclen = path_appendbkslash_if_dir(setup->cursrc);

    /* if src ends with a '\' then append *.* */
    if (setup->cursrc[cursrclen - 1] == '\\') {
      sv_strcat(setup->cursrc, "*.*");
    }

    /* remember where the path in cursrc ends */
    for (t = 0; setup->cursrc[t] != 0; t++) {
      if (setup->cursrc[t] == '\\') pathendoffset = t + 1;
    }

    /* */
    if (findfirst(dta, setup->cursrc, 0) != 0) {
      continue;
    }

    do {
      char appendflag;
      if (dta->attr & DOS_ATTR_DIR) continue; /* skip directories */

      /* compute full path/name of the file */
      sv_strcpy(setup->cursrc + pathendoffset, dta->fname);

      /* if there was no destination, then YOU are the destination now!
       * this handles situations like COPY a.txt+b.txt+c.txt */
      if (setup->dst[0] == 0) {
        sv_strcpy(setup->dst, setup->cursrc);
        setup->dstlen = sv_strlen(setup->dst);
        copiedcount_in++;
        copiedcount_out++;
        continue;
      }

      /* is dst ending with a backslash? then append fname to it */
      if (setup->dst[setup->dstlen - 1] == '\\') sv_strcpy(setup->dst + setup->dstlen, dta->fname);

      /* now cursrc contains the full source and dst contains the full dest... COPY TIME! */

      /* if dst file exists already -> overwrite it or append?
          - if dst is a dir (dstlen-1 points at a \\) -> overwrite
          - otherwise: if copiedcount_in==0 overwrite, else append */
      output(setup->cursrc);
      if ((setup->dst[setup->dstlen - 1] == '\\') || (copiedcount_in == 0)) {
        appendflag = 0;
        output(" > ");
        copiedcount_out++;
      } else {
        appendflag = 1;
        output(" >> ");
      }
      outputnl(setup->dst);

      t = cmd_copy_internal(setup->dst, 0, setup->cursrc, 0, appendflag, (setup->databufseg_custom != 0)?MK_FP(setup->databufseg_custom, 0):setup->databuf, setup->databufsz);
      if (t != 0) {
        nls_outputnl_doserr(t);
        /* free memory block if it is not the static BUFF */
        if (setup->databufseg_custom != 0) dos_freememseg(setup->databufseg_custom);
        return(CMD_FAIL);
      }

      copiedcount_in++;
    } while (findnext(dta) == 0);

  }

  ustoa(setup->databuf, copiedcount_out, 0, '0');
  sv_strcpy(setup->databuf + 8, svarlang_str(38,9)); /* "% file(s) copied" */
  sv_insert_str_in_str(setup->databuf + 8, setup->databuf);
  outputnl(setup->databuf + 8);

  /* free memory block if it is not the static BUFF */
  if (setup->databufseg_custom != 0) dos_freememseg(setup->databufseg_custom);

  return(CMD_OK);
}

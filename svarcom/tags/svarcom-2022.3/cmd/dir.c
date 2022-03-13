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
 * dir
 *
 * Displays a list of files and subdirectories in a directory.
 *
 * DIR [drive:][path][filename] [/P] [/W] [/A[:]attributes] [/O[[:]sortorder]] [/S] [/B] [/L]
 *
 * /P Pauses after each screenful of information.
 * /W Uses wide list format.
 *
 * /A Displays file with specified attributes:
 *     D Directories           R Read-only files     H Hidden files
 *     A Ready for archiving   S System files        - prefix meaning "not"
 *
 * /O List files in sorted order:
 *     N by name            S by size              E by extension
 *     D by date            G group dirs first     - prefix to reverse order
 *
 * /S Displays files in specified directory and all subdirectories.
 * /B Uses bare format (no heading information or summary)
 * /L Uses lowercases
 */

/* NOTE: /A attributes are matched in an exclusive way, ie. only files with
 *       the specified attributes are matched. This is different from how DOS
 *       itself matches attributes hence DIR cannot rely on the attributes
 *       filter within FindFirst.
 *
 * NOTE: Multiple /A are not supported - only the last one is significant.
 */

#define WCOLWIDTH 15  /* width of a column in wide mode output */


/* fills freebytes with free bytes for drv (A=0, B=1, etc)
 * returns DOS ERR code on failure */
static unsigned short cmd_dir_df(unsigned long *freebytes, unsigned char drv) {
  unsigned short res = 0;
  unsigned short sects_per_clust = 0, avail_clusts = 0, bytes_per_sect = 0;

  _asm {
    push ax
    push bx
    push cx
    push dx

    mov ah, 0x36  /* DOS 2+ -- Get Disk Free Space */
    mov dl, [drv] /* A=1, B=2, etc (0 = DEFAULT DRIVE) */
    inc dl
    int 0x21      /* AX=sects_per_clust, BX=avail_clusts, CX=bytes_per_sect, DX=tot_clusters */
    cmp ax, 0xffff /* AX=0xffff on error (invalid drive) */
    jne COMPUTEDF
    mov [res], 0x0f /* fill res with DOS error code 15 ("invalid drive") */
    jmp DONE

    COMPUTEDF:
    /* freebytes = AX * BX * CX */
    mov [sects_per_clust], ax
    mov [avail_clusts], bx
    mov [bytes_per_sect], cx

    DONE:
    pop dx
    pop cx
    pop bx
    pop ax
  }

  /* multiple steps to avoid uint16 overflow */
  *freebytes = sects_per_clust;
  *freebytes *= avail_clusts;
  *freebytes *= bytes_per_sect;

  return(res);
}


static void dir_pagination(unsigned short *availrows) {
  *availrows -= 1;
  if (*availrows == 0) {
    press_any_key();
    *availrows = screen_getheight() - 1;
  }
}


/* parse an attr list like "Ar-hS" and fill bitfield into attrfilter_may and attrfilter_must.
 * /AHS   -> adds S and H to mandatory attribs ("must")
 * /A-S   -> removes S from allowed attribs ("may")
 * returns non-zero on error. */
static int dir_parse_attr_list(const char *arg, unsigned char *attrfilter_may, unsigned char *attrfilter_must) {
  for (; *arg != 0; arg++) {
    unsigned char curattr;
    char not;
    if (*arg == '-') {
      not = 1;
      arg++;
    } else {
      not = 0;
    }
    switch (*arg) {
      case 'd':
      case 'D':
        curattr = DOS_ATTR_DIR;
        break;
      case 'r':
      case 'R':
        curattr = DOS_ATTR_RO;
        break;
      case 'a':
      case 'A':
        curattr = DOS_ATTR_ARC;
        break;
      case 'h':
      case 'H':
        curattr = DOS_ATTR_HID;
        break;
      case 's':
      case 'S':
        curattr = DOS_ATTR_SYS;
        break;
      default:
        return(-1);
    }
    /* update res bitfield */
    if (not) {
      *attrfilter_may &= ~curattr;
    } else {
      *attrfilter_must |= curattr;
    }
  }
  return(0);
}


#define DIR_ATTR_DEFAULT (DOS_ATTR_RO | DOS_ATTR_DIR | DOS_ATTR_ARC)

static enum cmd_result cmd_dir(struct cmd_funcparam *p) {
  const char *filespecptr = NULL;
  struct DTA *dta = (void *)0x80; /* set DTA to its default location at 80h in PSP */
  unsigned short i;
  unsigned short availrows;  /* counter of available rows on display (used for /P) */
  unsigned short wcols = screen_getwidth() / WCOLWIDTH; /* number of columns in wide mode */
  unsigned char wcolcount;
  struct nls_patterns *nls = (void *)(p->BUFFER + (p->BUFFERSZ / 2));
  char *buff2 = p->BUFFER + (p->BUFFERSZ / 2) + sizeof(*nls);
  unsigned long summary_fcount = 0;
  unsigned long summary_totsz = 0;
  unsigned char drv = 0;
  unsigned char attrfilter_may = DIR_ATTR_DEFAULT;
  unsigned char attrfilter_must = 0;

  #define DIR_FLAG_PAUSE  1
  #define DIR_FLAG_RECUR  4
  #define DIR_FLAG_LCASE  8
  unsigned char flags = 0;

  #define DIR_OUTPUT_NORM 1
  #define DIR_OUTPUT_WIDE 2
  #define DIR_OUTPUT_BARE 3
  unsigned char format = DIR_OUTPUT_NORM;

  if (cmd_ishlp(p)) {
    nls_outputnl(37,0); /* "Displays a list of files and subdirectories in a directory" */
    outputnl("");
    nls_outputnl(37,1); /* "DIR [drive:][path][filename] [/P] [/W] [/A[:]attributes] [/O[[:]sortorder]] [/S] [/B] [/L]" */
    outputnl("");
    nls_outputnl(37,2); /* "/P Pauses after each screenful of information" */
    nls_outputnl(37,3); /* "/W Uses wide list format" */
    outputnl("");
    nls_outputnl(37,4); /* "/A Displays files with specified attributes:" */
    nls_outputnl(37,5); /* "    D Directories            R Read-only files        H Hidden files" */
    nls_outputnl(37,6); /* "    A Ready for archiving    S System files           - prefix meaning "not"" */
    outputnl("");
    nls_outputnl(37,7); /* "/O List files in sorted order:" */
    nls_outputnl(37,8); /* "    N by name                S by size                E by extension" */
    nls_outputnl(37,9); /* "    D by date                G group dirs first       - prefix to reverse order" */
    outputnl("");
    nls_outputnl(37,10); /* "/S Displays files in specified directory and all subdirectories" */
    nls_outputnl(37,11); /* "/B Uses bare format (no heading information or summary)" */
    nls_outputnl(37,12); /* "/L Uses lowercases" */
    return(CMD_OK);
  }

  i = nls_getpatterns(nls);
  if (i != 0) nls_outputnl_doserr(i);

  /* parse command line */
  for (i = 0; i < p->argc; i++) {
    if (p->argv[i][0] == '/') {
      const char *arg = p->argv[i] + 1;
      char neg = 0;
      /* detect negations and get actual argument */
      if (*arg == '-') {
        neg = 1;
        arg++;
      }
      /* */
      switch (*arg) {
        case 'a':
        case 'A':
          arg++;
          /* preset defaults */
          attrfilter_may = DIR_ATTR_DEFAULT;
          attrfilter_must = 0;
          /* /-A only allowed without further parameters (used to cancel possible previous /Asmth) */
          if (neg) {
            if (*arg != 0) {
              nls_outputnl_err(0, 2); /* invalid switch */
              return(CMD_FAIL);
            }
          } else {
            /* skip colon if present */
            if (*arg == ':') arg++;
            /* start with "allow everything" */
            attrfilter_may = (DOS_ATTR_ARC | DOS_ATTR_DIR | DOS_ATTR_HID | DOS_ATTR_SYS | DOS_ATTR_RO);
            if (dir_parse_attr_list(arg, &attrfilter_may, &attrfilter_must) != 0) {
              nls_outputnl_err(0, 3); /* invalid parameter format */
              return(CMD_FAIL);
            }
          }
          break;
        case 'b':
        case 'B':
          format = DIR_OUTPUT_BARE;
          break;
        case 'l':
        case 'L':
          flags |= DIR_FLAG_LCASE;
          break;
        case 'o':
        case 'O':
          /* TODO */
          outputnl("/O NOT IMPLEMENTED YET");
          return(CMD_FAIL);
          break;
        case 'p':
        case 'P':
          flags |= DIR_FLAG_PAUSE;
          if (neg) flags &= (0xff ^ DIR_FLAG_PAUSE);
          break;
        case 's':
        case 'S':
          /* TODO */
          outputnl("/S NOT IMPLEMENTED YET");
          return(CMD_FAIL);
          break;
        case 'w':
        case 'W':
          format = DIR_OUTPUT_WIDE;
          break;
        default:
          nls_outputnl_err(0, 2); /* invalid switch */
          return(CMD_FAIL);
      }
    } else {  /* filespec */
      if (filespecptr != NULL) {
        nls_outputnl_err(0, 4); /* too many parameters */
        return(CMD_FAIL);
      }
      filespecptr = p->argv[i];
    }
  }

  if (filespecptr == NULL) filespecptr = ".";

  availrows = screen_getheight() - 2;

  /* special case: "DIR drive:" (truename() fails on "C:" under MS-DOS 6.0) */
  if ((filespecptr[0] != 0) && (filespecptr[1] == ':') && (filespecptr[2] == 0)) {
    if ((filespecptr[0] >= 'a') && (filespecptr[0] <= 'z')) {
      p->BUFFER[0] = filespecptr[0] - ('a' - 1);
    } else {
      p->BUFFER[0] = filespecptr[0] - ('A' - 1);
    }
    i = curpathfordrv(p->BUFFER, p->BUFFER[0]);
  } else {
    i = file_truename(filespecptr, p->BUFFER);
  }
  if (i != 0) {
    nls_outputnl_doserr(i);
    return(CMD_FAIL);
  }

  if (format != DIR_OUTPUT_BARE) {
    drv = p->BUFFER[0];
    if (drv >= 'a') {
      drv -= 'a';
    } else {
      drv -= 'A';
    }
    cmd_vol_internal(drv, buff2);
    sprintf(buff2, svarlang_str(37,20)/*"Directory of %s"*/, p->BUFFER);
    /* trim at first '?', if any */
    for (i = 0; buff2[i] != 0; i++) if (buff2[i] == '?') buff2[i] = 0;
    outputnl(buff2);
    outputnl("");
    availrows -= 3;
  }

  /* if dir: append a backslash (also get its len) */
  i = path_appendbkslash_if_dir(p->BUFFER);

  /* if ends with a \ then append ????????.??? */
  if (p->BUFFER[i - 1] == '\\') strcat(p->BUFFER, "????????.???");

  /* ask DOS for list of files, but only with allowed attribs */
  i = findfirst(dta, p->BUFFER, attrfilter_may);
  if (i != 0) {
    nls_outputnl_doserr(i);
    return(CMD_FAIL);
  }

  wcolcount = 0; /* may be used for columns counting with wide mode */

  do {
    /* if mandatory attribs are requested, filter them now */
    if ((attrfilter_must & dta->attr) != attrfilter_must) continue;

    /* if file contains attributes that are not allowed -> skip */
    if ((~attrfilter_may & dta->attr) != 0) continue;

    /* turn string lcase (/L) */
    if (flags & DIR_FLAG_LCASE) _strlwr(dta->fname); /* OpenWatcom extension, probably does not care about NLS so results may be odd with non-A-Z characters... */

    summary_fcount++;
    if ((dta->attr & DOS_ATTR_DIR) == 0) summary_totsz += dta->size;

    switch (format) {
      case DIR_OUTPUT_NORM:
        /* print fname-space-extension (unless it's "." or "..", then print as-is) */
        if (dta->fname[0] == '.') {
          output(dta->fname);
          i = strlen(dta->fname);
          while (i++ < 12) output(" ");
        } else {
          file_fname2fcb(buff2, dta->fname);
          memmove(buff2 + 9, buff2 + 8, 4);
          buff2[8] = ' ';
          output(buff2);
        }
        output(" ");
        /* either <DIR> or right aligned 10-chars byte size */
        memset(buff2, ' ', 10);
        if (dta->attr & DOS_ATTR_DIR) {
          strcpy(buff2 + 10, svarlang_str(37,21));
        } else {
          _ultoa(dta->size, buff2 + 10, 10); /* OpenWatcom extension */
        }
        output(buff2 + strlen(buff2) - 10);
        /* two spaces and NLS DATE */
        buff2[0] = ' ';
        buff2[1] = ' ';
        nls_format_date(buff2 + 2, dta->date_yr + 1980, dta->date_mo, dta->date_dy, nls);
        output(buff2);

        /* one space and NLS TIME */
        nls_format_time(buff2 + 1, dta->time_hour, dta->time_min, 0xff, nls);
        outputnl(buff2);
        break;

      case DIR_OUTPUT_WIDE: /* display in columns of 12 chars per item */
        i = strlen(dta->fname);
        if (dta->attr & DOS_ATTR_DIR) {
          i += 2;
          output("[");
          output(dta->fname);
          output("]");
        } else {
          output(dta->fname);
        }
        while (i++ < WCOLWIDTH) output(" ");
        if (++wcolcount == wcols) {
          wcolcount = 0;
          outputnl("");
        } else {
          availrows++; /* wide mode is the only one that does not write one line per file */
        }
        break;

      case DIR_OUTPUT_BARE:
        outputnl(dta->fname);
        break;
    }

    if (flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);

  } while (findnext(dta) == 0);

  if (wcolcount != 0) {
    outputnl(""); /* in wide mode make sure to end on a clear row */
    if (flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);
  }

  /* print out summary (unless bare output mode) */
  if (format != DIR_OUTPUT_BARE) {
    unsigned short alignpos;
    /* x file(s) */
    memset(buff2, ' ', 13); /* 13 is the max len of a 32 bit number with thousand separators (4'000'000'000) */
    i = nls_format_number(buff2 + 13, summary_fcount, nls);
    alignpos = sprintf(buff2 + 13 + i, " %s ", svarlang_str(37,22)/*"file(s)"*/);
    output(buff2 + i);
    /* xxxx bytes */
    i = nls_format_number(buff2 + 13, summary_totsz, nls);
    output(buff2 + i);
    output(" ");
    nls_outputnl(37,23); /* "bytes" */
    if (flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);
    /* xxxx bytes free */
    i = cmd_dir_df(&summary_totsz, drv);
    if (i != 0) nls_outputnl_doserr(i);
    alignpos += 13 + 13;
    memset(buff2, ' ', alignpos); /* align the freebytes value to same column as totbytes */
    i = nls_format_number(buff2 + alignpos, summary_totsz, nls);
    output(buff2 + i);
    output(" ");
    nls_outputnl(37,24); /* "bytes free" */
    if (flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);
  }

  return(CMD_OK);
}

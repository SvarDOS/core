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


/* a "tiny" DTA is a DTA that is stripped from bytes that are not needed for
 * DIR operations */
_Packed struct TINYDTA {
/*  char reserved[21];
  unsigned char attr; */
  unsigned short time_sec2:5;
  unsigned short time_min:6;
  unsigned short time_hour:5;
  unsigned short date_dy:5;
  unsigned short date_mo:4;
  unsigned short date_yr:7;
  unsigned long size;
/*  char fname[13]; */
  char fname[12];
};


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


/* compare attributes in a DTA node to mandatory and optional attributes. returns 1 on match, 0 otherwise */
static int filter_attribs(const struct DTA *dta, unsigned char attrfilter_must, unsigned char attrfilter_may) {
  /* if mandatory attribs are requested, filter them now */
  if ((attrfilter_must & dta->attr) != attrfilter_must) return(0);

  /* if file contains attributes that are not allowed -> skip */
  if ((~attrfilter_may & dta->attr) != 0) return(0);

  return(1);
}


static struct {
  struct TINYDTA far *dtabuf_root;
  char order[8]; /* GNESD values (ucase = lower first ; lcase = higher first) */
  unsigned char sortownia[256]; /* collation table (used for NLS-aware sorts) */
} glob_sortcmp_dat;


/* translates an order string like "GNE-S" into values fed into the order[]
 * table of glob_sortcmp_dat. returns 0 on success, non-zero otherwise. */
static int dir_process_order_directive(const char *ordstring) {
  const char *gnesd = "gnesd"; /* must be lower case */
  int ordi, orderi = 0, i;

  /* tabula rasa */
  glob_sortcmp_dat.order[0] = 0;

  /* /O alone is a short hand for /OGN */
  if (*ordstring == 0) {
    glob_sortcmp_dat.order[0] = 'G';
    glob_sortcmp_dat.order[1] = 'N';
    glob_sortcmp_dat.order[2] = 0;
  }

  /* stupid MSDOS compatibility ("DIR /O:GNE") */
  if (*ordstring == ':') ordstring++;

  /* parsing */
  for (ordi = 0; ordstring[ordi] != 0; ordi++) {
    if (ordstring[ordi] == '-') {
      if ((ordstring[ordi + 1] == '-') || (ordstring[ordi + 1] == 0)) return(-1);
      continue;
    }
    if (orderi == sizeof(glob_sortcmp_dat.order)) return(-1);

    for (i = 0; gnesd[i] != 0; i++) {
      if ((ordstring[ordi] | 32) == gnesd[i]) { /* | 32 is lcase-ing the char */
        if ((ordi > 0) && (ordstring[ordi - 1] == '-')) {
          glob_sortcmp_dat.order[orderi] = gnesd[i];
        } else {
          glob_sortcmp_dat.order[orderi] = gnesd[i] ^ 32;
        }
        orderi++;
        break;
      }
    }
    if (gnesd[i] == 0) return(-1);
  }

  return(0);
}


static int sortcmp(const void *dtaid1, const void *dtaid2) {
  struct TINYDTA far *dta1 = &(glob_sortcmp_dat.dtabuf_root[*((unsigned short *)dtaid1)]);
  struct TINYDTA far *dta2 = &(glob_sortcmp_dat.dtabuf_root[*((unsigned short *)dtaid2)]);
  char *ordconf = glob_sortcmp_dat.order;

  /* debug stuff
  {
    int i;
    printf("%lu vs %lu | ", dta1->size, dta2->size);
    for (i = 0; dta1->fname[i] != 0; i++) printf("%c", dta1->fname[i]);
    printf(" vs ");
    for (i = 0; dta2->fname[i] != 0; i++) printf("%c", dta2->fname[i]);
    printf("\n");
  } */

  for (;;) {
    int r = -1;
    if (*ordconf & 32) r = 1;

    switch (*ordconf | 32) {
      case 'g': /* sort by type (directories first, then files) */
        if ((dta1->time_sec2 & DOS_ATTR_DIR) > (dta2->time_sec2 & DOS_ATTR_DIR)) return(0 - r);
        if ((dta1->time_sec2 & DOS_ATTR_DIR) < (dta2->time_sec2 & DOS_ATTR_DIR)) return(r);
        break;
      case ' ': /* default (last resort) sort: by name */
      case 'e': /* sort by extension */
      case 'n': /* sort by filename */
      {
        const char far *f1 = dta1->fname;
        const char far *f2 = dta2->fname;
        int i, limit = 12;
        /* special handling for '.' and '..' entries */
        if ((f1[0] == '.') && (f2[0] != '.')) return(0 - r);
        if ((f2[0] == '.') && (f1[0] != '.')) return(r);

        if ((*ordconf | 32) == 'e') {
          /* fast-forward to extension or end of filename */
          while ((*f1 != 0) && (*f1 != '.')) f1++;
          while ((*f2 != 0) && (*f2 != '.')) f2++;
          limit = 4; /* TINYDTA structs are not nul-terminated */
        }
        /* cmp */
        for (i = 0; i < limit; i++) {
          if ((glob_sortcmp_dat.sortownia[(unsigned char)(*f1)]) < (glob_sortcmp_dat.sortownia[(unsigned char)(*f2)])) return(0 - r);
          if ((glob_sortcmp_dat.sortownia[(unsigned char)(*f1)]) > (glob_sortcmp_dat.sortownia[(unsigned char)(*f2)])) return(r);
          if (*f1 == 0) break;
          f1++;
          f2++;
        }
      }
        break;
      case 's': /* sort by size */
        if (dta1->size > dta2->size) return(r);
        if (dta1->size < dta2->size) return(0 - r);
        break;
      case 'd': /* sort by date */
        if (dta1->date_yr < dta2->date_yr) return(0 - r);
        if (dta1->date_yr > dta2->date_yr) return(r);
        if (dta1->date_mo < dta2->date_mo) return(0 - r);
        if (dta1->date_mo > dta2->date_mo) return(r);
        if (dta1->date_dy < dta2->date_dy) return(0 - r);
        if (dta1->date_dy > dta2->date_dy) return(r);
        if (dta1->time_hour < dta2->time_hour) return(0 - r);
        if (dta1->time_hour > dta2->time_hour) return(r);
        if (dta1->time_min < dta2->time_min) return(0 - r);
        if (dta1->time_min > dta2->time_min) return(r);
        break;
    }

    if (*ordconf == 0) break;
    ordconf++;
  }

  return(0);
}


#define DIR_ATTR_DEFAULT (DOS_ATTR_RO | DOS_ATTR_DIR | DOS_ATTR_ARC)

struct dirrequest {
  unsigned char attrfilter_may;
  unsigned char attrfilter_must;
  const char *filespecptr;

  #define DIR_FLAG_PAUSE  1
  #define DIR_FLAG_RECUR  4
  #define DIR_FLAG_LCASE  8
  #define DIR_FLAG_SORT  16
  unsigned char flags;

  #define DIR_OUTPUT_NORM 1
  #define DIR_OUTPUT_WIDE 2
  #define DIR_OUTPUT_BARE 3
  unsigned char format;
};


static int dir_parse_cmdline(struct dirrequest *req, const char **argv) {
  for (; *argv != NULL; argv++) {
    if (*argv[0] == '/') {
      const char *arg = *argv + 1;
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
          req->attrfilter_may = DIR_ATTR_DEFAULT;
          req->attrfilter_must = 0;
          /* /-A only allowed without further parameters (used to cancel possible previous /Asmth) */
          if (neg) {
            if (*arg != 0) {
              nls_outputnl_err(0, 2); /* invalid switch */
              return(-1);
            }
          } else {
            /* skip colon if present */
            if (*arg == ':') arg++;
            /* start with "allow everything" */
            req->attrfilter_may = (DOS_ATTR_ARC | DOS_ATTR_DIR | DOS_ATTR_HID | DOS_ATTR_SYS | DOS_ATTR_RO);
            if (dir_parse_attr_list(arg, &(req->attrfilter_may), &(req->attrfilter_must)) != 0) {
              nls_outputnl_err(0, 3); /* invalid parameter format */
              return(-1);
            }
          }
          break;
        case 'b':
        case 'B':
          req->format = DIR_OUTPUT_BARE;
          break;
        case 'l':
        case 'L':
          req->flags |= DIR_FLAG_LCASE;
          break;
        case 'o':
        case 'O':
          if (neg) {
            req->flags &= (0xff ^ DIR_FLAG_SORT);
            break;
          }
          if (dir_process_order_directive(arg+1) != 0) {
            nls_output_err(0, 3); /* invalid parameter format */
            output(": ");
            outputnl(arg);
            return(-1);
          }
          req->flags |= DIR_FLAG_SORT;
          break;
        case 'p':
        case 'P':
          req->flags |= DIR_FLAG_PAUSE;
          if (neg) req->flags &= (0xff ^ DIR_FLAG_PAUSE);
          break;
        case 's':
        case 'S':
          /* TODO */
          outputnl("/S NOT IMPLEMENTED YET");
          return(-1);
          break;
        case 'w':
        case 'W':
          req->format = DIR_OUTPUT_WIDE;
          break;
        default:
          nls_outputnl_err(0, 2); /* invalid switch */
          return(-1);
      }
    } else {  /* filespec */
      if (req->filespecptr != NULL) {
        nls_outputnl_err(0, 4); /* too many parameters */
        return(-1);
      }
      req->filespecptr = *argv;
    }
  }

  return(0);
}


static enum cmd_result cmd_dir(struct cmd_funcparam *p) {
  struct DTA *dta = (void *)0x80; /* set DTA to its default location at 80h in PSP */
  struct TINYDTA far *dtabuf = NULL; /* used to buffer results when sorting is enabled */
  unsigned short dtabufcount = 0;
  unsigned short i;
  unsigned short availrows;  /* counter of available rows on display (used for /P) */
  unsigned short screenw = screen_getwidth();
  unsigned short wcols = screenw / WCOLWIDTH; /* number of columns in wide mode */
  unsigned char wcolcount;
  struct {
    struct nls_patterns nls;
    char buff64[64];
    char path[128];
    unsigned short orderidx[65535 / sizeof(struct TINYDTA)];
  } *buf = (void *)(p->BUFFER);
  unsigned long summary_fcount = 0;
  unsigned long summary_totsz = 0;
  unsigned char drv = 0;
  struct dirrequest req;

  /* make sure there's no risk of buffer overflow */
  if (sizeof(buf) > p->BUFFERSZ) {
    outputnl("INTERNAL MEM ERROR IN " __FILE__);
    return(CMD_FAIL);
  }

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

  /* zero out glob_sortcmp_dat and init the collation table */
  bzero(&glob_sortcmp_dat, sizeof(glob_sortcmp_dat));
  for (i = 0; i < 256; i++) {
    glob_sortcmp_dat.sortownia[i] = i;
    /* sorting should be case-insensitive */
    if ((i >= 'A') && (i <= 'Z')) glob_sortcmp_dat.sortownia[i] |= 32;
  }

  /* try to replace (or complement) my naive collation table with an NLS-aware
   * version provided by the kernel (or NLSFUNC)
   * see https://github.com/SvarDOS/bugz/issues/68 for some thoughts */
  {
    _Packed struct nlsseqtab {
      unsigned char id;
      unsigned short taboff;
      unsigned short tabseg;
    } collat;
    void *colptr = &collat;
    unsigned char errflag = 1;
    _asm {
      push ax
      push bx
      push cx
      push dx
      push di
      push es

      mov ax, 0x6506  /* DOS 3.3+ - Get collating sequence table */
      mov bx, 0xffff  /* code page, FFFFh = "current" */
      mov cx, 5       /* size of buffer at ES:DI */
      mov dx, 0xffff  /* country id, FFFFh = "current" */
      push ds
      pop es          /* ES:DI = address of buffer for the 5-bytes struct */
      mov di, colptr
      int 0x21
      jc FAIL
      xor al, al
      mov errflag, al
      FAIL:

      pop es
      pop di
      pop dx
      pop cx
      pop bx
      pop ax
    }

    if ((errflag == 0) && (collat.id == 6)) {
      unsigned char far *ptr = MK_FP(collat.tabseg, collat.taboff);
      unsigned short count = *(unsigned short far *)ptr;
#ifdef DIR_DUMPNLSCOLLATE
      printf("NLS AT %04X:%04X (%u elements)\n", collat.tabseg, collat.taboff, count);
#endif
      if (count <= 256) { /* you never know */
        ptr += 2; /* skip the count header */
        for (i = 0; i < count; i++) {
          glob_sortcmp_dat.sortownia[i] = ptr[i];
#ifdef DIR_DUMPNLSCOLLATE
          printf(" %03u", ptr[i]);
          if ((i & 15) == 15) {
            printf("\n");
            fflush(stdout);
          }
#endif
        }
      }
    }
  }

  i = nls_getpatterns(&(buf->nls));
  if (i != 0) nls_outputnl_doserr(i);

  /* disable usage of thousands separator on narrow screens */
  if (screenw < 80) buf->nls.thousep[0] = 0;

  /*** PARSING COMMAND LINE STARTS *******************************************/

  /* init req with some defaults */
  bzero(&req, sizeof(req));
  req.attrfilter_may = DIR_ATTR_DEFAULT;
  req.format = DIR_OUTPUT_NORM;

  /* process DIRCMD first (so it can be overidden by user's cmdline) */
  {
  const char far *dircmd = env_lookup_val(p->env_seg, "DIRCMD");
  if (dircmd != NULL) {
    const char *argvptrs[32];
    cmd_explode(buf->buff64, dircmd, argvptrs);
    if ((dir_parse_cmdline(&req, argvptrs) != 0) || (req.filespecptr != NULL)) {
      nls_output(255, 10);/* bad environment */
      output(" - ");
      outputnl("DIRCMD");
      return(CMD_FAIL);
    }
  }
  }

  /* parse user's command line */
  if (dir_parse_cmdline(&req, p->argv) != 0) return(CMD_FAIL);

  /* if no filespec provided, then it's about the current directory */
  if (req.filespecptr == NULL) req.filespecptr = ".";

  /*** PARSING COMMAND LINE DONE *********************************************/


  availrows = screen_getheight() - 2;

  /* special case: "DIR drive:" (truename() fails on "C:" under MS-DOS 6.0) */
  if ((req.filespecptr[0] != 0) && (req.filespecptr[1] == ':') && (req.filespecptr[2] == 0)) {
    if ((req.filespecptr[0] >= 'a') && (req.filespecptr[0] <= 'z')) {
      buf->path[0] = req.filespecptr[0] - ('a' - 1);
    } else {
      buf->path[0] = req.filespecptr[0] - ('A' - 1);
    }
    i = curpathfordrv(buf->path, buf->path[0]);
  } else {
    i = file_truename(req.filespecptr, buf->path);
  }
  if (i != 0) {
    nls_outputnl_doserr(i);
    return(CMD_FAIL);
  }

  if (req.format != DIR_OUTPUT_BARE) {
    drv = buf->path[0];
    if (drv >= 'a') {
      drv -= 'a';
    } else {
      drv -= 'A';
    }
    cmd_vol_internal(drv, buf->buff64);
    sprintf(buf->buff64, svarlang_str(37,20)/*"Directory of %s"*/, buf->path);
    /* trim at first '?', if any */
    for (i = 0; buf->buff64[i] != 0; i++) if (buf->buff64[i] == '?') buf->buff64[i] = 0;
    outputnl(buf->buff64);
    outputnl("");
    availrows -= 3;
  }

  /* if dir: append a backslash (also get its len) */
  i = path_appendbkslash_if_dir(buf->path);

  /* if ends with a \ then append ????????.??? */
  if (buf->path[i - 1] == '\\') strcat(buf->path, "????????.???");

  /* ask DOS for list of files, but only with allowed attribs */
  i = findfirst(dta, buf->path, req.attrfilter_may);
  if (i != 0) {
    nls_outputnl_doserr(i);
    return(CMD_FAIL);
  }

  /* if sorting is involved, then let's buffer all results (and sort them) */
  if (req.flags & DIR_FLAG_SORT) {
    /* allocate a memory buffer - try several sizes until one succeeds */
    const unsigned short memsz[] = {65500, 32000, 16000, 8000, 4000, 2000, 1000, 0};
    unsigned short max_dta_bufcount = 0;
    for (i = 0; memsz[i] != 0; i++) {
      dtabuf = _fmalloc(memsz[i]);
      if (dtabuf != NULL) break;
    }

    if (dtabuf == NULL) {
      nls_outputnl_doserr(8); /* out of memory */
      return(CMD_FAIL);
    }

    /* remember the address so I can free it afterwards */
    glob_sortcmp_dat.dtabuf_root = dtabuf;

    /* compute the amount of DTAs I can buffer */
    max_dta_bufcount = memsz[i] / sizeof(struct TINYDTA);
    /* printf("max_dta_bufcount = %u\n", max_dta_bufcount); */

    do {
      /* filter out files with uninteresting attributes */
      if (filter_attribs(dta, req.attrfilter_must, req.attrfilter_may) == 0) continue;

      /* normalize "size" of directories to zero because kernel returns garbage
       * sizes for directories which might confuse the sorting routine later */
      if (dta->attr & DOS_ATTR_DIR) dta->size = 0;

      _fmemcpy(&(dtabuf[dtabufcount]), ((char *)dta) + 22, sizeof(struct TINYDTA));

      /* save attribs in sec field, otherwise zero it (this field is not
       * displayed and dropping the attr field saves 2 bytes per entry) */
      dtabuf[dtabufcount++].time_sec2 = (dta->attr & 31);

      /* do I have any space left? */
      if (dtabufcount == max_dta_bufcount) {
        //TODO some kind of user notification might be nice here
        //outputnl("TOO MANY ENTRIES FOR SORTING! LIST IS UNSORTED");
        break;
      }

    } while (findnext(dta) == 0);

    /* no match? kein gluck! (this can happen when filtering attribs with /A:xxx
     * because while findfirst() succeeds, all entries can be rejected) */
    if (dtabufcount == 0) {
      nls_outputnl_doserr(2); /* "File not found" */
      return(CMD_FAIL);
    }

    /* sort the list - the tricky part is that my array is a far address while
     * qsort works only with near pointers, so I have to use an ugly (and
     * global) auxiliary table */
    for (i = 0; i < dtabufcount; i++) buf->orderidx[i] = i;
    qsort(buf->orderidx, dtabufcount, 2, &sortcmp);

    /* preload first entry (last from orderidx, since entries are sorted in reverse) */
    dtabufcount--;
    _fmemcpy(((unsigned char *)dta) + 22, &(dtabuf[buf->orderidx[dtabufcount]]), sizeof(struct TINYDTA));
    dta->attr = dtabuf[buf->orderidx[dtabufcount]].time_sec2; /* restore attr from the abused time_sec2 field */
  }

  wcolcount = 0; /* may be used for columns counting with wide mode */

  for (;;) {

    /* filter out attributes (skip if entry comes from buffer, then it was already veted) */
    if (filter_attribs(dta, req.attrfilter_must, req.attrfilter_may) == 0) goto NEXT_ENTRY;

    /* turn string lcase (/L) */
    if (req.flags & DIR_FLAG_LCASE) _strlwr(dta->fname); /* OpenWatcom extension, probably does not care about NLS so results may be odd with non-A-Z characters... */

    summary_fcount++;
    if ((dta->attr & DOS_ATTR_DIR) == 0) summary_totsz += dta->size;

    switch (req.format) {
      case DIR_OUTPUT_NORM:
        /* print fname-space-extension (unless it's "." or "..", then print as-is) */
        if (dta->fname[0] == '.') {
          output(dta->fname);
          i = strlen(dta->fname);
          while (i++ < 12) output(" ");
        } else {
          file_fname2fcb(buf->buff64, dta->fname);
          memmove(buf->buff64 + 9, buf->buff64 + 8, 4);
          buf->buff64[8] = ' ';
          output(buf->buff64);
        }
        output(" ");
        /* either <DIR> or right aligned 13 or 10 chars byte size, depending
         * on the presence of a thousands delimiter (max 2'000'000'000) */
        {
          unsigned short szlen = 10 + (strlen(buf->nls.thousep) * 3);
          memset(buf->buff64, ' ', 16);
          if (dta->attr & DOS_ATTR_DIR) {
            strcpy(buf->buff64 + szlen, svarlang_str(37,21));
          } else {
            nls_format_number(buf->buff64 + 12, dta->size, &(buf->nls));
          }
          output(buf->buff64 + strlen(buf->buff64) - szlen);
        }
        /* one spaces and NLS DATE */
        buf->buff64[0] = ' ';
        if (screenw >= 80) {
          nls_format_date(buf->buff64 + 1, dta->date_yr + 1980, dta->date_mo, dta->date_dy, &(buf->nls));
        } else {
          nls_format_date(buf->buff64 + 1, (dta->date_yr + 80) % 100, dta->date_mo, dta->date_dy, &(buf->nls));
        }
        output(buf->buff64);

        /* one space and NLS TIME */
        nls_format_time(buf->buff64 + 1, dta->time_hour, dta->time_min, 0xff, &(buf->nls));
        outputnl(buf->buff64);
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

    if (req.flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);

    NEXT_ENTRY:
    /* take next entry, either from buf or disk */
    if (dtabufcount > 0) {
      dtabufcount--;
      _fmemcpy(((unsigned char *)dta) + 22, &(dtabuf[buf->orderidx[dtabufcount]]), sizeof(struct TINYDTA));
      dta->attr = dtabuf[buf->orderidx[dtabufcount]].time_sec2; /* restore attr from the abused time_sec2 field */
    } else {
      if (findnext(dta) != 0) break;
    }

  }

  if (wcolcount != 0) {
    outputnl(""); /* in wide mode make sure to end on a clear row */
    if (req.flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);
  }

  /* print out summary (unless bare output mode) */
  if (req.format != DIR_OUTPUT_BARE) {
    unsigned short alignpos;
    unsigned short uint32maxlen = 14; /* 13 is the max len of a 32 bit number with thousand separators (4'000'000'000) */
    if (screenw < 80) uint32maxlen = 10;

    /* x file(s) (maximum of files in a FAT-32 directory is 65'535) */
    memset(buf->buff64, ' ', 8);
    i = nls_format_number(buf->buff64 + 8, summary_fcount, &(buf->nls));
    alignpos = sprintf(buf->buff64 + 8 + i, " %s ", svarlang_str(37,22)/*"file(s)"*/);
    output(buf->buff64 + i);
    /* xxxx bytes */
    memset(buf->buff64, ' ', 14);
    i = nls_format_number(buf->buff64 + uint32maxlen, summary_totsz, &(buf->nls));
    output(buf->buff64 + i + 1);
    output(" ");
    nls_outputnl(37,23); /* "bytes" */
    if (req.flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);

    /* xxxx bytes free */
    i = cmd_dir_df(&summary_totsz, drv);
    if (i != 0) nls_outputnl_doserr(i);
    alignpos += 8 + uint32maxlen;
    memset(buf->buff64, ' ', alignpos); /* align the freebytes value to same column as totbytes */
    i = nls_format_number(buf->buff64 + alignpos, summary_totsz, &(buf->nls));
    output(buf->buff64 + i + 1);
    output(" ");
    nls_outputnl(37,24); /* "bytes free" */
    if (req.flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);
  }

  /* free the buffer memory (if used) */
  if (glob_sortcmp_dat.dtabuf_root != NULL) _ffree(glob_sortcmp_dat.dtabuf_root);

  return(CMD_OK);
}

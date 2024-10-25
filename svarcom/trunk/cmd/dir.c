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
 *
 * about /S - recursive DIR on specified (or current) path and subdirectories:
 * prerequisite: some sort of mechanism that works as a stack pile of DTAs
 *
 * /S logic:
 * 1. do a FindFirst on current directory
 * 2. do FindNext calls in a loop, if a DIR entry is encountered, remember its
 *    name and put a copy of the current DTA on stack, then continue the
 *    listing without further interruption
 * 3. if a new DIR was discovered, do a FindFirst on it and jmp to 2.
 *    if no DIR found, then go to 4.
 * 4. look on the stack for a DTA.
 *    if any found, pop it and jmp to 2.
 *    otherwise job is done, exit.
 */

/* NOTE: /A attributes are matched in an exclusive way, ie. only files with
 *       the specified attributes are matched. This is different from how DOS
 *       itself matches attributes hence DIR cannot rely on the attributes
 *       filter within FindFirst.
 *
 * NOTE: Multiple /A are not supported - only the last one is significant.
 */


/* width of a column in wide mode output: 15 chars is the MINIMUM because
 * directories are enclosed in [BRACKETS] and they may have an extension, too.
 * Hence "[12345678.123]" is the longest we can get. Plus a delimiter space. */
#define WCOLWIDTH 15


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


static void far *cmd_dir_farmalloc(unsigned short segcount);
#pragma aux cmd_dir_farmalloc = \
"mov ah, 0x48" \
"int 0x21" \
"jnc DONE" \
"xor ax, ax" \
"DONE:" \
"xor bx, bx" \
"mov es, ax" \
parm [bx] \
modify [ax] \
value [es bx] \


static void cmd_dir_farfree(void far *ptr);
#pragma aux cmd_dir_farfree = \
"mov ah, 0x49" \
"int 0x21" \
parm [es ax] \
modify [ax]



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


static void dir_print_dirprefix(const char *p) {
  unsigned char t, lastbkslash;
  char buff[2] = {0, 0};

  /* find the last backslash of path */
  lastbkslash = 0;
  for (t = 0; p[t] != 0; t++) {
    if (p[t] == '\\') lastbkslash = t;
  }

  /* print path until last bkslash */
  do {
    *buff = *p;
    output(buff);
    p++;
  } while (lastbkslash-- != 0);
}


/* print the "Directory of C:\ABC\.... string using a buffer with possible
 * file pattern garbage trailing */
static void dir_print_dirof(const char *p, unsigned short *availrows, unsigned char pagination) {
  char buff[2] = {0, 0};
  const char *dirof = svarlang_str(37,20); /* Directory of % */

  outputnl("");
  if (pagination) dir_pagination(availrows);

  /* print string until % */
  while ((*dirof != 0) && (*dirof != '%')) {
    *buff = *dirof;
    output(buff);
    dirof++;
  }

  if (*dirof != '%') return;
  dirof++;

  /* print path until last bkslash */
  dir_print_dirprefix(p);

  /* print the rest of the dirof string */
  while (*dirof != 0) {
    *buff = *dirof;
    output(buff);
    dirof++;
  }

  outputnl("");
  if (pagination) dir_pagination(availrows);
  outputnl("");
  if (pagination) dir_pagination(availrows);
}


/* add a new dirname to path, C:\XXX\*.EXE + YYY -> C:\XXX\YYY\*.EXE */
static void path_add(char *path, const char *dirname) {
  short i, ostatni = -1;
  //printf("path_add(%s,%s) -> ", path, dirname);
  /* find the last backslash */
  for (i = 0; path[i] != 0; i++) {
    if (path[i] == '\\') ostatni = i;
  }
  /* abort on error */
  if (ostatni == -1) return;
  /* do the trick */
  /* move ending to the right */
  memcpy_rtl(path + ostatni + sv_strlen(dirname) + 1, path + ostatni, sv_strlen(path + ostatni) + 1);
  /* fill in the space with dirname */
  memcpy_ltr(path + ostatni + 1, dirname, sv_strlen(dirname));
  //printf("'%s'\n", path);
}


/* take back last dir from path, C:\XXX\YYY\*.EXE -> C:\XXX\*.EXE */
static void path_back(char *path) {
  short i, ostatni = -1, przedostatni = -1;
  //printf("path_back(%s) -> ", path);
  /* find the two last backslashes */
  for (i = 0; path[i] != 0; i++) {
    if (path[i] == '\\') {
      przedostatni = ostatni;
      ostatni = i;
    }
  }
  /* abort on error */
  if (przedostatni == -1) return;
  /* do the trick */
  memcpy_ltr(path + przedostatni, path + ostatni, 1 + i - ostatni);
  //printf("'%s'\n", path);
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


static int sortcmp(const struct TINYDTA far *dta1, const struct TINYDTA far *dta2) {
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


/* sort function for DIR /O (selection sort) */
static void cmd_dir_sort(struct TINYDTA far *dta, unsigned short dtacount) {
  int i, t, smallest;
  for (i = 0; i < (dtacount - 1); i++) {
    // find "smallest" entry
    smallest = i;
    for (t = i + 1; t < dtacount; t++) {
      if (sortcmp(dta + t, dta + smallest) < 0) smallest = t;
    }
    // if smallest different than current found then swap
    if (smallest != i) {
      struct TINYDTA entry;
      memcpy_ltr_far(&entry, dta + i, sizeof(struct TINYDTA));
      memcpy_ltr_far(dta + i, dta + smallest, sizeof(struct TINYDTA));
      memcpy_ltr_far(dta + smallest, &entry, sizeof(struct TINYDTA));
    }
  }
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
          req->flags |= DIR_FLAG_RECUR;
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


static void dir_print_summary_files(char *buff64, unsigned short uint32maxlen, unsigned long summary_totsz, unsigned long summary_fcount, unsigned short *availrows, unsigned char flags, const struct nls_patterns *nls) {
  unsigned short i;

  /* x file(s) (maximum of files in a FAT-32 directory is 65'535) */
  sv_memset(buff64, ' ', 8);
  buff64[8] = 0;
  i = nls_format_number(buff64 + 8, summary_fcount, nls);
  output(buff64 + i);
  output(" ");
  output(svarlang_str(37,22)); /* "file(s)" */
  output(" ");

  /* xxxx bytes */
  sv_memset(buff64, ' ', 14);
  i = nls_format_number(buff64 + uint32maxlen, summary_totsz, nls);
  output(buff64 + i + 1);
  output(" ");
  nls_outputnl(37,23); /* "bytes" */
  if (flags & DIR_FLAG_PAUSE) dir_pagination(availrows);
}


/* max amount of files to sort - limited by the memory block I will allocate
 * to store the TINYDTA of each entry */
#define MAX_SORTABLE_FILES (65500 / sizeof(struct TINYDTA))

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
    struct DTA dtastack[64]; /* used for /S, max number of subdirs in DOS5 is 42 (A/B/C/...) */
    unsigned char dtastacklen;
  } *buf;
  unsigned long summary_recurs_fcount = 0; /* used for /s global summary */
  unsigned long summary_recurs_totsz = 0;  /* used for /s global summary */
  unsigned long summary_fcount;
  unsigned long summary_totsz;
  unsigned char drv = 0;
  struct dirrequest req;
  unsigned short summary_alignpos = sv_strlen(svarlang_str(37,22)) + 2;
  unsigned short uint32maxlen = 14; /* 13 is the max len of a 32 bit number with thousand separators (4'000'000'000) */
  if (screenw < 80) uint32maxlen = 10;

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
    goto GAMEOVER;
  }

  /* allocate buf */
  buf = calloc(sizeof(*buf), 1);
  if (buf == NULL) {
    nls_output_err(255, 8); /* insufficient memory */
    goto GAMEOVER;
  }

  /* zero out glob_sortcmp_dat and init the collation table */
  sv_bzero(&glob_sortcmp_dat, sizeof(glob_sortcmp_dat));
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
  sv_bzero(&req, sizeof(req));
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
      goto GAMEOVER;
    }
  }
  }

  /* parse user's command line */
  if (dir_parse_cmdline(&req, p->argv) != 0) goto GAMEOVER;

  /*** PARSING COMMAND LINE DONE *********************************************/

  /* if no filespec provided, then it's about the current directory */
  if (req.filespecptr == NULL) req.filespecptr = ".";

  availrows = screen_getheight() - 1;

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
    goto GAMEOVER;
  }

  /* volume label and serial */
  if (req.format != DIR_OUTPUT_BARE) {
    drv = buf->path[0];
    if (drv >= 'a') {
      drv -= 'a';
    } else {
      drv -= 'A';
    }
    cmd_vol_internal(drv, buf->buff64);
    availrows -= 2;
  }

  NEXT_ITER: /* re-entry point for /S recursing */

  summary_fcount = 0;
  summary_totsz = 0;

  /* if dir: append a backslash (also get its len) */
  i = path_appendbkslash_if_dir(buf->path);

  /* if ends with a \ then append ????????.??? */
  if (buf->path[i - 1] == '\\') sv_strcat(buf->path, "????????.???");

  /* ask DOS for list of files, but only with allowed attribs */
  i = findfirst(dta, buf->path, req.attrfilter_may);

  /* print "directory of" unless /B or /S mode with no match */
  if ((req.format != DIR_OUTPUT_BARE) && (((req.flags & DIR_FLAG_RECUR) == 0) || (i == 0))) {
    dir_print_dirof(buf->path, &availrows, req.flags & DIR_FLAG_PAUSE);
  }

  /* if no file match then abort */
  if (i != 0) {
    if (req.flags & DIR_FLAG_RECUR) goto CHECK_RECURS;
    nls_outputnl_doserr(i);
    goto GAMEOVER;
  }

  /* if sorting is involved, then let's buffer all results (and sort them) */
  if (req.flags & DIR_FLAG_SORT) {
    /* allocate a memory buffer - try several sizes until one succeeds */
    unsigned short max_dta_bufcount;

    /* compute the amount of DTAs I can buffer */
    for (max_dta_bufcount = MAX_SORTABLE_FILES; max_dta_bufcount != 0; max_dta_bufcount /= 2) {
      dtabuf = cmd_dir_farmalloc(max_dta_bufcount * sizeof(struct TINYDTA) / 16);
      if (dtabuf != NULL) break;
    }
    /* printf("max_dta_bufcount = %u\n", max_dta_bufcount); */

    if (dtabuf == NULL) {
      nls_outputnl_doserr(8); /* out of memory */
      goto GAMEOVER;
    }

    /* remember the address so I can free it afterwards */
    glob_sortcmp_dat.dtabuf_root = dtabuf;

    do {
      /* filter out files with uninteresting attributes */
      if (filter_attribs(dta, req.attrfilter_must, req.attrfilter_may) == 0) continue;

      /* /B hides . and .. entries */
      if ((req.format == DIR_OUTPUT_BARE) && (dta->fname[0] == '.')) continue;

      /* normalize "size" of directories to zero because kernel returns garbage
       * sizes for directories which might confuse the sorting routine later */
      if (dta->attr & DOS_ATTR_DIR) dta->size = 0;

      memcpy_ltr_far(&(dtabuf[dtabufcount]), ((char *)dta) + 22, sizeof(struct TINYDTA));

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
      if (req.flags & DIR_FLAG_RECUR) goto CHECK_RECURS;
      nls_outputnl_doserr(2); /* "File not found" */
      goto GAMEOVER;
    }

    /* sort the list */
    cmd_dir_sort(dtabuf, dtabufcount);

    /* preload first entry (last, since entries are sorted in reverse) */
    dtabufcount--;
    memcpy_ltr_far(((unsigned char *)dta) + 22, dtabuf + dtabufcount, sizeof(struct TINYDTA));
    dta->attr = dtabuf[dtabufcount].time_sec2; /* restore attr from the abused time_sec2 field */
  }

  wcolcount = 0; /* may be used for columns counting with wide mode */

  for (;;) {

    /* filter out attributes (skip if entry comes from buffer, then it was already veted) */
    if (filter_attribs(dta, req.attrfilter_must, req.attrfilter_may) == 0) goto NEXT_ENTRY;

    /* /B hides . and .. entries */
    if ((req.format == DIR_OUTPUT_BARE) && (dta->fname[0] == '.')) continue;

    /* turn string lcase (/L) - naive method, only low-ascii */
    if (req.flags & DIR_FLAG_LCASE) {
      char *s = dta->fname;
      while (*s != 0) {
        if ((*s >= 'A') && (*s <= 'Z')) *s |= 0x20;
        s++;
      }
    }

    summary_fcount++;
    if ((dta->attr & DOS_ATTR_DIR) == 0) summary_totsz += dta->size;

    switch (req.format) {
      case DIR_OUTPUT_NORM:
        /* print fname-space-extension (unless it's "." or "..", then print as-is) */
        if (dta->fname[0] == '.') {
          output(dta->fname);
          i = sv_strlen(dta->fname);
          while (i++ < 12) output(" ");
        } else {
          file_fname2fcb(buf->buff64, dta->fname);
          memcpy_rtl(buf->buff64 + 9, buf->buff64 + 8, 4);
          buf->buff64[8] = ' ';
          output(buf->buff64);
        }
        output(" ");
        /* either <DIR> or right aligned 13 or 10 chars byte size, depending
         * on the presence of a thousands delimiter (max 2'000'000'000) */
        {
          unsigned short szlen = 10 + (sv_strlen(buf->nls.thousep) * 3);
          sv_memset(buf->buff64, ' ', 16);
          if (dta->attr & DOS_ATTR_DIR) {
            sv_strcpy(buf->buff64 + szlen, svarlang_str(37,21));
          } else {
            nls_format_number(buf->buff64 + 12, dta->size, &(buf->nls));
          }
          output(buf->buff64 + sv_strlen(buf->buff64) - szlen);
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
        i = sv_strlen(dta->fname);
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
        /* if /B used in combination with /S then files are displayed with full path */
        if (req.flags & DIR_FLAG_RECUR) dir_print_dirprefix(buf->path);
        outputnl(dta->fname);
        break;
    }

    if (req.flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);

    NEXT_ENTRY:
    /* take next entry, either from buf or disk */
    if (dtabufcount > 0) {
      dtabufcount--;
      memcpy_ltr_far(((unsigned char *)dta) + 22, dtabuf + dtabufcount, sizeof(struct TINYDTA));
      dta->attr = dtabuf[dtabufcount].time_sec2; /* restore attr from the abused time_sec2 field */
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
    dir_print_summary_files(buf->buff64, uint32maxlen, summary_totsz, summary_fcount, &availrows, req.flags, &(buf->nls));
  }

  /* update global counters in case /s is used */
  summary_recurs_fcount += summary_fcount;
  summary_recurs_totsz += summary_totsz;

  /* /S processing */
  CHECK_RECURS:
  /* if /S then look for a subdir */
  if (req.flags & DIR_FLAG_RECUR) {
    /* do the findfirst on *.* instead of reusing the user filter */
    char *s;
    char backup[4];
    //printf("orig path='%s' new=", buf->path);
    for (s = buf->path; *s != 0; s++);
    for (; s[-1] != '\\'; s--);
    memcpy_ltr(backup, s, 4);
    memcpy_ltr(s, "*.*", 4);
    //printf("'%s'\n", buf->path);
    if (findfirst(dta, buf->path, DOS_ATTR_DIR) == 0) {
      memcpy_ltr(s, backup, 4);
      for (;;) {
        if ((dta->fname[0] != '.') && (dta->attr & DOS_ATTR_DIR)) break;
        if (findnext(dta) != 0) goto NOSUBDIR;
      }
      //printf("GOT DIR (/S): '%s'\n", dta->fname);
      /* add dir to path and redo scan */
      memcpy_ltr(&(buf->dtastack[buf->dtastacklen]), dta, sizeof(struct DTA));
      buf->dtastacklen++;
      path_add(buf->path, dta->fname);
      goto NEXT_ITER;
    }
    memcpy_ltr(s, backup, 4);
  }
  NOSUBDIR:

  while (buf->dtastacklen > 0) {
    /* rewind path one directory back, pop the next dta and do a FindNext */
    path_back(buf->path);
    buf->dtastacklen--;
    TRYNEXTENTRY:
    if (findnext(&(buf->dtastack[buf->dtastacklen])) != 0) continue;
    if ((buf->dtastack[buf->dtastacklen].attr & DOS_ATTR_DIR) == 0) goto TRYNEXTENTRY;
    if (buf->dtastack[buf->dtastacklen].fname[0] == '.') goto TRYNEXTENTRY;
    /* something found -> add dir to path and redo scan */
    path_add(buf->path, buf->dtastack[buf->dtastacklen].fname);
    goto NEXT_ITER;
  }

  /* print out disk space available (unless bare output mode) */
  if (req.format != DIR_OUTPUT_BARE) {
    /* if /s mode then print also global stats */
    if (req.flags & DIR_FLAG_RECUR) {
      if (summary_recurs_fcount == 0) {
        file_truename(req.filespecptr, buf->path);
        dir_print_dirof(buf->path, &availrows, req.flags & DIR_FLAG_PAUSE);
        nls_outputnl_doserr(2); /* "File not found" */
        goto GAMEOVER;
      } else {
        outputnl("");
        if (req.flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);
        nls_outputnl(37,25); /* Total files listed: */
        if (req.flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);
        dir_print_summary_files(buf->buff64, uint32maxlen, summary_recurs_totsz, summary_recurs_fcount, &availrows, req.flags, &(buf->nls));
      }
    }
    /* xxxx bytes free */
    i = cmd_dir_df(&summary_totsz, drv);
    if (i != 0) nls_outputnl_doserr(i);
    sv_memset(buf->buff64, ' ', summary_alignpos + 8 + uint32maxlen); /* align the freebytes value to same column as totbytes */
    i = nls_format_number(buf->buff64 + summary_alignpos + 8 + uint32maxlen, summary_totsz, &(buf->nls));
    output(buf->buff64 + i + 1);
    output(" ");
    nls_outputnl(37,24); /* "bytes free" */
    if (req.flags & DIR_FLAG_PAUSE) dir_pagination(&availrows);
  }

  GAMEOVER:

  /* free the buffer memory (if used) */
  if (glob_sortcmp_dat.dtabuf_root != NULL) cmd_dir_farfree(glob_sortcmp_dat.dtabuf_root);

  free(buf);
  return(CMD_OK);
}

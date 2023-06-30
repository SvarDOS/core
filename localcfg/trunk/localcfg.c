/*
 * Locales configuration for SvarDOS
 *
 * Copyright (C) Mateusz Viste 2015-2023
 *
 * MIT license
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h> /* atoi() */
#include <string.h> /* strchr */

#include "svarlang.h"

#include "country.h"

#define PVER "20230630"
#define PDATE "2015-2023"


enum NLS_STRINGS {
  NLS_HLP_VER           = 0x0000,
  NLS_HLP_DESC          = 0x0001,
  NLS_HLP_USAGE         = 0x0002,
  NLS_HLP_OPTIONS       = 0x0003,
  NLS_HLP_COUNTRY       = 0x000A,
  NLS_HLP_CP            = 0x000B,
  NLS_HLP_DECIM         = 0x000C,
  NLS_HLP_THOUS         = 0x000D,
  NLS_HLP_DATESEP       = 0x000E,
  NLS_HLP_DATEFMT       = 0x000F,
  NLS_HLP_TIMESEP       = 0x0010,
  NLS_HLP_TIMEFMT       = 0x0011,
  NLS_HLP_CURR          = 0x0012,
  NLS_HLP_CURRPOS0      = 0x0013,
  NLS_HLP_CURRPOS1      = 0x0014,
  NLS_HLP_CURRPOS2      = 0x0015,
  NLS_HLP_CURRSPC       = 0x0016,
  NLS_HLP_CURRPREC      = 0x0017,
  NLS_HLP_YESNO         = 0x0018,
  NLS_HLP_INFOLOC1      = 0x0032,
  NLS_HLP_INFOLOC2      = 0x0033,

  NLS_INFO_COUNTRY      = 0x0700,
  NLS_INFO_CODEPAGE     = 0x0701,
  NLS_INFO_DECSEP       = 0x0702,
  NLS_INFO_THOUSEP      = 0x0703,
  NLS_INFO_DATEFMT      = 0x0704,
  NLS_INFO_TIMEFMT      = 0x0705,
  NLS_INFO_YESNO        = 0x0706,
  NLS_INFO_CURREXAMPLE  = 0x0707,
  NLS_MAKESURE          = 0x0709,

  NLS_ERR_FILEPATHTWICE = 0x0900,
  NLS_ERR_BADPATH       = 0x0901,
  NLS_ERR_READFAIL      = 0x0902,
  NLS_ERR_INVPARAM      = 0x0903,
  NLS_ERR_INVFORMAT     = 0x0904,
  NLS_ERR_NOTLOCALCFG   = 0x0905
};


static void output(const char *s) {
  _asm {
    /* set cx to strlen(s) */
    push ds
    pop es
    mov di, s
    xor al, al
    cld
    mov cx, 0xff
    repne scasb  /* compare ES:DI with AL, inc DI until match */
    mov cx, di
    sub cx, s
    dec cx
    /* output via DOS */
    mov ah, 0x40  /* write to handle */
    mov bx, 1     /* 1=stdout */
    mov dx, s
    int 0x21
  }
}


static void crlf(void) {
  output("\r\n");
}


static void outputnl(const char *s) {
  output(s);
  crlf();
}


static void nls_put(enum NLS_STRINGS id) {
  output(svarlang_strid(id));
}


static void nls_puts(enum NLS_STRINGS id) {
  nls_put(id);
  crlf();
}


static void about(void) {
  output("localcfg ");
  nls_put(NLS_HLP_VER);
  outputnl(" " PVER ", (C) " PDATE " Mateusz Viste");
  nls_puts(NLS_HLP_DESC);
  crlf();
  nls_puts(NLS_HLP_USAGE);
  crlf();
  nls_puts(NLS_HLP_OPTIONS);
  crlf();
  nls_puts(NLS_HLP_COUNTRY);
  nls_puts(NLS_HLP_CP);
  nls_puts(NLS_HLP_DECIM);
  nls_puts(NLS_HLP_THOUS);
  nls_puts(NLS_HLP_DATESEP);
  nls_puts(NLS_HLP_DATEFMT);
  nls_puts(NLS_HLP_TIMESEP);
  nls_puts(NLS_HLP_TIMEFMT);
  nls_puts(NLS_HLP_CURR);
  nls_puts(NLS_HLP_CURRPOS0);
  nls_puts(NLS_HLP_CURRPOS1);
  nls_puts(NLS_HLP_CURRPOS2);
  nls_puts(NLS_HLP_CURRSPC);
  nls_puts(NLS_HLP_CURRPREC);
  nls_puts(NLS_HLP_YESNO);
  crlf();
  nls_puts(NLS_HLP_INFOLOC1);
  nls_puts(NLS_HLP_INFOLOC2);
}


static char *datestring(char *result, struct country *c) {
  switch (c->CTYINFO.datefmt) {
    case COUNTRY_DATE_MDY:
      sprintf(result, "12%c31%c1990", c->CTYINFO.datesep[0], c->CTYINFO.datesep[0]);
      break;
    case COUNTRY_DATE_DMY:
      sprintf(result, "31%c12%c1990", c->CTYINFO.datesep[0], c->CTYINFO.datesep[0]);
      break;
    case COUNTRY_DATE_YMD:
    default:
      sprintf(result, "1990%c12%c31", c->CTYINFO.datesep[0], c->CTYINFO.datesep[0]);
      break;
  }
  return(result);
}


static char *timestring(char *result, struct country *c) {
  if (c->CTYINFO.timefmt == COUNTRY_TIME12) {
    sprintf(result, "11%c59%c59 PM", c->CTYINFO.timesep[0], c->CTYINFO.timesep[0]);
  } else {
    sprintf(result, "23%c59%c59", c->CTYINFO.timesep[0], c->CTYINFO.timesep[0]);
  }
  return(result);
}


static char *currencystring(char *result, struct country *c) {
  char decimalpart[16];
  char space[2] = {0, 0};
  char decsym[8];
  char cursym[8];
  decimalpart[0] = '1';
  decimalpart[1] = '2';
  decimalpart[2] = '3';
  decimalpart[3] = '4';
  decimalpart[4] = '5';
  decimalpart[5] = '6';
  decimalpart[6] = '7';
  decimalpart[7] = '8';
  decimalpart[8] = '9';
  decimalpart[9] = 0;
  /* prepare the decimal string first */
  if (c->CTYINFO.currprec < 9) {
    decimalpart[c->CTYINFO.currprec] = 0;
  }
  /* prepare the currency space string */
  if (c->CTYINFO.currspace != 0) {
    space[0] = ' ';
  }
  /* prepare the currency and decimal symbols */
  if (c->CTYINFO.currdecsym != 0) { /* currency replaces the decimal point */
    sprintf(decsym, "%s", c->CTYINFO.currsym);
    cursym[0] = 0;
  } else {
    sprintf(decsym, "%c", c->CTYINFO.decimal[0]);
    sprintf(cursym, "%s", c->CTYINFO.currsym);
  }
  if (c->CTYINFO.currprec == 0) decsym[0] = 0;
  /* compute the final string */
  if (c->CTYINFO.currpos == 0) { /* currency precedes value */
    sprintf(result, "%s%s99%s%s", cursym, space, decsym, decimalpart);
  } else { /* currency follows value or replaces decimal symbol */
    sprintf(result, "99%s%s%s%s", decsym, decimalpart, space, cursym);
  }
  return(result);
}


/* checks if str starts with prefix. returns 0 if so, non-zero otherwise. */
static int stringstartswith(char *str, char *prefix) {
  for (;;) {
    /* end of prefix means success */
    if (*prefix == 0) return(0);
    /* otherwise there is no match */
    if (*str != *prefix) return(-1);
    /* if match good so far, look at next char */
    str += 1;
    prefix += 1;
  }
}


/* processes an argument. returns 0 on success, non-zero otherwise. */
static int processarg(char *arg, struct country *c) {
  char *value;
  int intvalue;
  /* an option must start with a '/' */
  if (arg[0] != '/') return(-1);
  arg += 1; /* skip the slash */
  /* find where the value starts */
  value = strchr(arg, ':');
  /* if no value present, fail */
  if (value == NULL) return(-2);
  value += 1;
  if (*value == 0) return(-3);
  /* interpret the option now */
  if (stringstartswith(arg, "country:") == 0) {
    intvalue = atoi(value);
    if ((intvalue > 0) && (intvalue < 1000)) {
      c->CTYINFO.id = intvalue;
      return(0);
    }
  } else if (stringstartswith(arg, "cp:") == 0) {
    intvalue = atoi(value);
    if ((intvalue > 0) && (intvalue < 1000)) {
      c->CTYINFO.codepage = intvalue;
      return(0);
    }
  } else if (stringstartswith(arg, "decim:") == 0) {
    if (value[1] == 0) { /* value must be exactly one character */
      c->CTYINFO.decimal[0] = *value;
      return(0);
    }
  } else if (stringstartswith(arg, "thous:") == 0) {
    if (value[1] == 0) { /* value must be exactly one character */
      c->CTYINFO.thousands[0] = *value;
      return(0);
    }
  } else if (stringstartswith(arg, "datesep:") == 0) {
    if (value[1] == 0) { /* value must be exactly one character */
      c->CTYINFO.datesep[0] = *value;
      return(0);
    }
  } else if (stringstartswith(arg, "timesep:") == 0) {
    if (value[1] == 0) { /* value must be exactly one character */
      c->CTYINFO.timesep[0] = *value;
      return(0);
    }
  } else if (stringstartswith(arg, "datefmt:") == 0) {
    if (strcmp(value, "MDY") == 0) {
      c->CTYINFO.datefmt = COUNTRY_DATE_MDY;
      return(0);
    } else if (strcmp(value, "DMY") == 0) {
      c->CTYINFO.datefmt = COUNTRY_DATE_DMY;
      return(0);
    } else if (strcmp(value, "YMD") == 0) {
      c->CTYINFO.datefmt = COUNTRY_DATE_YMD;
      return(0);
    }
  } else if (stringstartswith(arg, "timefmt:") == 0) {
    if (value[1] == 0) {
      if ((value[0] >= '0') && (value[0] <= '1')) {
        c->CTYINFO.timefmt = value[0] - '0';
        return(0);
      }
    }
  } else if (stringstartswith(arg, "curr:") == 0) {
    if (strlen(value) <= 4) {
      strcpy(c->CTYINFO.currsym, value);
      return(0);
    }
  } else if (stringstartswith(arg, "currpos:") == 0) {
    if (value[1] == 0) {
      if (value[0] == '0') {
        c->CTYINFO.currpos = 0;
        return(0);
      } else if (value[0] == '1') {
        c->CTYINFO.currpos = 1;
        return(0);
      } else if (value[0] == '2') {
        c->CTYINFO.currpos = 0;
        c->CTYINFO.currdecsym = 1;
        return(0);
      }
    }
  } else if (stringstartswith(arg, "currspc:") == 0) {
    if (value[1] == 0) {
      if ((value[0] >= '0') && (value[0] <= '1')) {
        c->CTYINFO.currspace = value[0] - '0';
        return(0);
      }
    }
  } else if (stringstartswith(arg, "currprec:") == 0) {
    if (value[1] == 0) {
      if ((value[0] >= '0') && (value[0] <= '9')) {
        c->CTYINFO.currprec = value[0] - '0';
        return(0);
      }
    }
  } else if (stringstartswith(arg, "yesno:") == 0) {
    /* string must be exactly 2 characters long */
    if ((value[0] != 0) && (value[1] != 0) && (value[2] == 0)) {
      c->YESNO.yes[0] = value[0];
      c->YESNO.no[0] = value[1];
      return(0);
    }
  }
  /* if I'm here, something went wrong */
  return(-4);
}


/* converts a path to its canonic representation, returns 0 on success
 * or DOS err on failure (invalid drive) */
static unsigned short file_truename(const char *dst, char *src) {
  unsigned short res = 0;
  _asm {
    push es
    mov ah, 0x60  /* query truename, DS:SI=src, ES:DI=dst */
    push ds
    pop es
    mov si, src
    mov di, dst
    int 0x21
    jnc DONE
    mov [res], ax
    DONE:
    pop es
  }
  return(res);
}


static void default_country_path(char *s) {
  char *dosdir = getenv("DOSDIR");
  size_t dosdirlen;
  s[0] = 0;
  if (dosdir == NULL) return;
  dosdirlen = strlen(dosdir);
  if (dosdirlen == 0) return;
  /* drop trailing backslash if present */
  if (dosdir[dosdirlen - 1] == '\\') dosdirlen--;
  /* copy dosdir to s and append the rest of the path */
  memcpy(s, dosdir, dosdirlen);
  strcpy(s + dosdirlen, "\\CFG\\COUNTRY.SYS");
}


int main(int argc, char **argv) {
  struct country cntdata;
  int changedflag;
  int x;
  static char fname[130];
  static char buff[64];

  svarlang_autoload("localcfg");

  /* scan argv looking for the path to country.sys */
  for (x = 1; x < argc; x++) {
    if (argv[x][0] != '/') {
      if (fname[0] != 0) {
        nls_puts(NLS_ERR_FILEPATHTWICE);
        return(1);
      }
      /* */
      if (file_truename(fname, argv[x]) != 0) {
        nls_puts(NLS_ERR_BADPATH);
        return(1);
      }
    } else if (strcmp(argv[x], "/?") == 0) { /* is it /? */
      about();
      return(1);
    }
  }

  /* if no file path provided, look into %DOSDIR%\CFG\COUNTRY.SYS */
  if (fname[0] == 0) default_country_path(fname);

  x = country_read(&cntdata, fname);
  if (x != 0) {
    if (x == COUNTRY_ERR_INV_FORMAT) {
      nls_puts(NLS_ERR_INVFORMAT);
    } else if (x == COUNTRY_ERR_NOT_LOCALCFG) {
      nls_puts(NLS_ERR_NOTLOCALCFG);
    } else {
      nls_puts(NLS_ERR_READFAIL);
    }
    return(2);
  }

  changedflag = 0;

  /* process command line arguments */
  for (x = 1; x < argc; x++) {
    if (argv[x][0] != '/') continue; /* skip country.sys filename (processed earlier) */
    changedflag++;
    if (processarg(argv[x], &cntdata) != 0) {
      nls_puts(NLS_ERR_INVPARAM);
      return(3);
    }
  }

  nls_put(NLS_INFO_COUNTRY);
  sprintf(buff, " %03d", cntdata.CTYINFO.id);
  outputnl(buff);
  nls_put(NLS_INFO_CODEPAGE);
  sprintf(buff, " %d", cntdata.CTYINFO.codepage);
  outputnl(buff);
  nls_put(NLS_INFO_DECSEP);
  sprintf(buff, " %c", cntdata.CTYINFO.decimal[0]);
  outputnl(buff);
  nls_put(NLS_INFO_THOUSEP);
  sprintf(buff, " %c", cntdata.CTYINFO.thousands[0]);
  outputnl(buff);
  nls_put(NLS_INFO_DATEFMT);
  output(" ");
  outputnl(datestring(buff, &cntdata));
  nls_put(NLS_INFO_TIMEFMT);
  output(" ");
  outputnl(timestring(buff, &cntdata));
  nls_put(NLS_INFO_YESNO);
  sprintf(buff, " %c/%c", cntdata.YESNO.yes[0], cntdata.YESNO.no[0]);
  outputnl(buff);
  nls_put(NLS_INFO_CURREXAMPLE);
  output(" ");
  outputnl(currencystring(buff, &cntdata));

  crlf();
  nls_puts(NLS_MAKESURE);
  sprintf(buff, "COUNTRY=%03d,%03d,", cntdata.CTYINFO.id, cntdata.CTYINFO.codepage);
  output(buff);
  outputnl(fname);

  /* if anything changed, write the new file */
  if (changedflag != 0) country_write(fname, &cntdata);

  return(0);
}

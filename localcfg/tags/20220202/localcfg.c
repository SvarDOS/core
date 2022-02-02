/*
 * Locales configuration for SvarDOS
 *
 * Copyright (C) Mateusz Viste 2015-2022
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

#include "country.h"

#define PVER "20220202"
#define PDATE "2015-2022"


static void about(void) {
  puts("localcfg ver " PVER " - locales configuration for DOS\n"
       "Copyright (C) " PDATE " Mateusz Viste\n"
       "\n"
       "localcfg creates or edits a custom COUNTRY.SYS-like file with your preferences.\n"
       "\n"
       "usage: localcfg [COUNTRY.SYS] [options]\n"
       "\n"
       "options allow to configure country locales to your likening, as follows:\n"
       "  /country:XX indicates your country code is XX (1 for USA, 33 for France, etc)\n"
       "  /cp:XXX     adapts country data for codepage XXX (example: '437')\n"
       "  /decim:X    reconfigures the decimal symbol to be 'X'");
  puts("  /thous:X    reconfigures the thousands symbol to be 'X'\n"
       "  /datesep:X  sets the date separator to 'X' (for example '/')\n"
       "  /datefmt:X  sets the date format, can be: MDY, DMY or YMD\n"
       "  /timesep:X  sets the time separator to 'X' (for example ':')\n"
       "  /timefmt:X  sets the time format: 0=12h with AM/PM or 1=24h\n"
       "  /curr:XXX   sets the currency to XXX (a string of 1 to 4 characters)\n"
       "  /currpos:X  sets the currency symbol position to X, where X is either");
  puts("              0=currency precedes the value, 1=currency follows the value and\n"
       "              2=currency replaces the decimal sign\n"
       "  /currspc:X  space between the currency and the value (0=no, 1=yes)\n"
       "  /currprec:X currency's precision (number of decimal digits, 0..9)\n"
       "  /yesno:XY   sets the 'Yes/No' letter to XY (default: YN)\n"
       "\n"
       "If COUNTRY.SYS location is not provided, then localcfg tries loading it\n"
       "from %DOSDIR%\\CFG\\COUNTRY.SYS\n"
      );
}


static char *datestring(struct country *c) {
  static char result[16];
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


static char *timestring(struct country *c) {
  static char result[16];
  if (c->CTYINFO.timefmt == COUNTRY_TIME12) {
    sprintf(result, "11%c59%c59 PM", c->CTYINFO.timesep[0], c->CTYINFO.timesep[0]);
  } else {
    sprintf(result, "23%c59%c59", c->CTYINFO.timesep[0], c->CTYINFO.timesep[0]);
  }
  return(result);
}


static char *currencystring(struct country *c) {
  static char result[16];
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

  /* scan argv looking for the path to country.sys */
  for (x = 1; x < argc; x++) {
    if (argv[x][0] != '/') {
      if (fname[0] != 0) {
        puts("ERROR: file path can be provided only once");
        return(1);
      }
      /* */
      if (file_truename(fname, argv[x]) != 0) {
        puts("ERROR: bad file path");
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
    printf("ERROR: failed to read the preference file [%d]\n", x);
    return(2);
  }

  changedflag = 0;

  /* process command line arguments */
  for (x = 1; x < argc; x++) {
    if (argv[x][0] != '/') continue; /* skip country.sys filename (processed earlier) */
    changedflag++;
    if (processarg(argv[x], &cntdata) != 0) {
      puts("ERROR: invalid parameter syntax");
      return(3);
    }
  }

  printf("Country intl code.....: %03d\n", cntdata.CTYINFO.id);
  printf("Codepage..............: %d\n", cntdata.CTYINFO.codepage);
  printf("Decimal separator.....: %c\n", cntdata.CTYINFO.decimal[0]);
  printf("Thousands separator...: %c\n", cntdata.CTYINFO.thousands[0]);
  printf("Date format...........: %s\n", datestring(&cntdata));
  printf("Time format...........: %s\n", timestring(&cntdata));
  printf("Yes/No letters........: %c/%c\n", cntdata.YESNO.yes[0], cntdata.YESNO.no[0]);
  printf("Currency example......: %s\n", currencystring(&cntdata));

  printf("\n"
         "Make sure that your CONFIG.SYS contains this directive:\n"
         "COUNTRY=%03d,%03d,%s\n\n", cntdata.CTYINFO.id, cntdata.CTYINFO.codepage, fname);

  /* if anything changed, write the new file */
  if (changedflag != 0) country_write(fname, &cntdata);

  return(0);
}

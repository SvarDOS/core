/* This file provides functions for parsing commands and their arguments

   Warning: parsecmd() will modify the cmdline string, so it won't be
   readable anymore in any other way other than via ptrtable[].
   This function returns the number of arguments that have been parsed,
   or -1 on parsing error.

   Copyright (C) 2012-2016 Mateusz Viste */

#include "version.h"

int parsecmd(char *cmdline, char **ptrtable, int maxargs) {
  int x = 0, argc = 0, state = 0;
  for (;;) {
    switch (cmdline[x]) { /* detect delimiter and non-delimiter chars */
      case 0x0: /* detect end of string */
        return(argc); /* return number of arguments */
      case ' ': /* space */
      case 0x9: /* tab */
      case 0xA: /* LF */
      case 0xD: /* CR */
        if (state != 0) { /* if awaiting for argument end */
          cmdline[x] = 0; /* terminate the substring */
          state = 0; /* switch to 'waiting for argument end' state */
        }
        break;
      default: /* anything that is not a delimiter */
        if (state == 0) { /* if awaiting for argument start */
          if (argc == maxargs) return(-1); /* look out for arg overflow */
          ptrtable[argc] = &cmdline[x]; /* save the address of the substring */
          argc += 1; /* increment the arguments count */
          state = 1; /* switch to 'waiting for argument' state */
        }
    }
    x += 1; /* move to the next character of cmdline */
  }
}

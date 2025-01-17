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
 * time [time]
 */


/* read a one or two digit number and write it to buff */
static int cmd_time_get_item(char *buff, const char *s) {
  unsigned short i;

  for (i = 0; i < 3; i++) {
    if ((s[i] < '0') || (s[i] > '9')) {
      buff[i] = 0;
      return(0);
    }
    buff[i] = s[i];
  }

  /* err */
  *buff = 0;
  return(-1);
}


/* parse a NULL-terminated string int hour, minutes and seconds, returns 0 on success
 * valid inputs: 0, 7, 5:5, 23:23, 17:54:45, 9p, 9:05, ...
 */
static int cmd_time_parse(const char *s, unsigned char *ho, unsigned char *mi, unsigned char *se, struct nls_patterns *nls) {
  unsigned short i;
  const char *ptrs[2] = {NULL, NULL}; /* minutes, seconds */
  char buff[3];
  char ampm = 0;

  *ho = -1;
  *mi = 0;
  *se = 0;

  /* validate input - must contain only chars 0-9, time separator and 'a' or 'p' */
  for (i = 0; s[i] != 0; i++) {
    switch (s[i]) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        break;
      case 'a':
      case 'A':
      case 'p':
      case 'P':
        /* these can be only at last position and never at the first */
        if ((s[i + 1] != 0) || (i == 0)) return(-1);
        ampm = s[i];
        if (ampm >= 'a') ampm -= ('a' - 'A');
        break;
      default:
        if ((s[i] != nls->timesep[0]) || (i == 0)) return(-1);
        if (ptrs[0] == NULL) {
          ptrs[0] = s + i + 1;
        } else if (ptrs[1] == NULL) {
          ptrs[1] = s + i + 1;
        } else { /* too many separators */
          return(-1);
        }
        break;
    }
  }

  /* read hour */
  if (cmd_time_get_item(buff, s) != 0) goto FAIL;
  if (atous(&i, buff) != 0) goto FAIL;
  *ho = i;

  /* if minutes provided, read them */
  if (ptrs[0] != NULL) {
    if (cmd_time_get_item(buff, ptrs[0]) != 0) goto FAIL;
    if (atous(&i, buff) != 0) goto FAIL;
    *mi = i;
  }

  /* if seconds provided, read them */
  if (ptrs[1] != NULL) {
    if (cmd_time_get_item(buff, ptrs[1]) != 0) goto FAIL;
    if (atous(&i, buff) != 0) goto FAIL;
    *se = i;
  }

  /* validate ranges */
  if ((*ho > 23) || (*mi > 59) || (*se > 59)) goto FAIL;

  /* am? */
  if ((ampm == 'A') && (*ho > 12)) goto FAIL;
  if ((ampm == 'A') && (*ho == 12)) *ho = 0; /* 12:00am is 00:00 (midnight) */

  /* pm? */
  if (ampm == 'P') {
    if (*ho > 12) goto FAIL;
    if (*ho < 12) *ho += 12;
  }

  return(0);

  FAIL:
  *ho = 255;
  return(-1);
}


static enum cmd_result cmd_time(struct cmd_funcparam *p) {
  struct nls_patterns *nls = (void *)(p->BUFFER);
  char *buff = p->BUFFER + sizeof(*nls);
  unsigned short i;
  unsigned char ho = 255, mi, se;

  if (cmd_ishlp(p)) {
    const char *hlpformats[] = {"14:00:00", "19:30", "03:05p", "17", "9a", NULL};
    nls_outputnl(22,0); /* "Displays or sets the system time." */
    outputnl("");
    nls_outputnl(22,1); /* "TIME [time]" */
    outputnl("");
    nls_outputnl(22,2); /* "Type TIME with no parameters to display (...) Examples:" */
    outputnl("");
    for (i = 0; hlpformats[i] != NULL; i++) {
      output("TIME ");
      outputnl(hlpformats[i]);
    }
    return(CMD_OK);
  }

  i = nls_getpatterns(nls);
  if (i != 0) {
    nls_outputnl_doserr(i);
    return(CMD_FAIL);
  }

  /* display current time if no args */
  if (p->argc == 0) {
    /* get cur time */
    dos_get_time(&ho, &mi, &se);

    buff[0] = ' ';
    nls_format_time(buff + 1, ho, mi, se, nls);
    nls_output(22,3); /* "Current time is" */
    outputnl(buff);
    ho = 255;
  } else if (p->argc == 1) { /* parse time if provided */
    if (cmd_time_parse(p->argv[0], &ho, &mi, &se, nls) != 0) {
      nls_outputnl(22,4); /* "Invalid time" */
      ho = 255;
    }
  } else {
    nls_outputnl(0, 4); /* "too many arguments" */
    return(CMD_FAIL);
  }

  /* ask for time if not provided or if input was malformed */
  while (ho == 255) {
    nls_output(22,5); /* "Enter new time:" */
    output(" ");
    /* collect user input into buff */
    _asm {
      push ax
      push bx
      push dx

      mov ah, 0x0a   /* DOS 1+ -- Buffered String Input */
      mov bx, buff
      mov dx, bx
      mov al, 16
      mov [bx], al   /* max input length */
      mov al, 1
      mov [bx+1], al /* zero out the "previous entry" length */
      int 0x21
      /* terminate the string with a NULL terminator */
      xor ax, ax
      inc bx
      mov al, [bx] /* read length of input string */
      mov bx, ax
      add bx, dx
      mov [bx+2], ah
      /* output a \n */
      mov ah, 2
      mov dl, 0x0A
      int 0x21

      pop dx
      pop bx
      pop ax
    }
    if (buff[1] == 0) break; /* empty string = do not change time */
    if (cmd_time_parse(buff + 2, &ho, &mi, &se, nls) == 0) break;
    nls_outputnl(22,4); /* "Invalid time" */
    return(CMD_FAIL);
  }

  if (ho != 255) {
    /* set time */
    _asm {
      push ax
      push bx
      push cx
      push dx

      mov ah, 0x2d   /* DOS 1+ -- Set DOS Time */
      mov ch, [ho]   /* hour (0-23) */
      mov cl, [mi]   /* minutes (0-59) */
      mov dh, [se]   /* seconds (0-59) */
      mov dl, 0      /* 1/100th seconds (0-99) */
      int 0x21

      pop dx
      pop cx
      pop bx
      pop ax
    }
  }

  return(CMD_OK);
}

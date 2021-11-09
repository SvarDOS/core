/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021 Mateusz Viste
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
static int cmd_time_parse(const char *s, signed char *ho, signed char *mi, signed char *se, struct nls_patterns *nls) {
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

  /* if minutes provided, read them */
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
  *ho = -1;
  return(-1);
}


static int cmd_time(struct cmd_funcparam *p) {
  struct nls_patterns *nls = (void *)(p->BUFFER);
  char *buff = p->BUFFER + (BUFFER_SIZE / 2);
  unsigned short i;
  signed char ho = -1, mi = -1, se = -1;

  if (cmd_ishlp(p)) {
    outputnl("Displays or sets the system time.");
    outputnl("");
    outputnl("TIME [time]");
    outputnl("");
    outputnl("Type TIME with no parameters to display the current time and a prompt for a");
    outputnl("new one. Press ENTER to keep the same time.");
    return(-1);
  }

  i = nls_getpatterns(nls);
  if (i != 0) {
    outputnl(doserr(i));
    return(-1);
  }

  /* display current time if no args */
  if (p->argc == 0) {
    /* get cur time */
    _asm {
      push ax
      push bx
      push cx
      push dx

      mov ah, 0x2c  /* DOS 1+ -- Query DOS Time */
      int 0x21      /* CH=hour CL=minutes DH=seconds DL=1/100sec */
      mov [ho], ch
      mov [mi], cl
      mov [se], dh

      pop dx
      pop cx
      pop bx
      pop ax
    }
    buff[0] = ' ';
    nls_format_time(buff + 1, ho, mi, se, nls);
    output("Current time is");
    outputnl(buff);
    ho = -1;
  } else { /* parse time if provided */
    if (cmd_time_parse(p->argv[0], &ho, &mi, &se, nls) != 0) {
      outputnl("Invalid time");
      ho = -1;
    }
  }

  /* ask for time if not provided or if input was malformed */
  while (ho < 0) {
    output("Enter new time:");
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
    outputnl("Invalid time");
  }

  if (ho >= 0) {
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

  return(-1);
}

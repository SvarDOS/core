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
 * date [date]
 */


/* parse a NULL-terminated string int hour, minutes and seconds, returns 0 on success
 * valid inputs: 0, 7, 5:5, 23:23, 17:54:45, 9p, 9:05, ...
 */
static int cmd_date_parse(const char *s, unsigned short *year, unsigned char *mo, unsigned char *dy, struct nls_patterns *nls) {
  unsigned short i;
  const char *ptrs[2] = {NULL, NULL};

  *year = 0;
  *mo = 0;
  *dy = 0;

  /* validate input - must contain only chars 0-9 and time separator */
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
      default:
        if ((s[i] != nls->datesep[0]) || (i == 0)) return(-1);
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

  /* did I get all separators? */
  if ((ptrs[0] == NULL) || (ptrs[1] == NULL)) goto FAIL;

  /* d/m/y order depends on NLS settings */
  switch (nls->dateformat) {
    case 0:  /* m/d/y */
      atous(&i, s);
      *mo = i;
      atous(&i, ptrs[0]);
      *dy = i;
      atous(year, ptrs[1]);
      break;
    case 1:  /* d/m/y */
      atous(&i, s);
      *dy = i;
      atous(&i, ptrs[0]);
      *mo = i;
      atous(year, ptrs[1]);
      break;
    default: /* y/m/d */
      atous(year, s);
      atous(&i, ptrs[0]);
      *mo = i;
      atous(&i, ptrs[1]);
      *dy = i;
      break;
  }

  return(0);

  FAIL:
  *year = 0;
  return(-1);
}


/* set system date, return 0 on success */
static int cmd_date_set(unsigned short year, unsigned char mo, unsigned char dy) {
  _asm {
    push ax
    push bx
    push cx
    push dx

    mov ax, 0x2b00 /* DOS 1+ -- Set DOS Date */
    mov cx, [year] /* year (1980-2099) */
    mov dh, [mo]   /* month (1-12) */
    mov dl, [dy]   /* day (1-31) */
    int 0x21       /* AL = 0 on success */
    cmp al, 0
    je DONE
    mov [year], 0
    DONE:

    pop dx
    pop cx
    pop bx
    pop ax
  }

  if (year == 0) return(-1);
  return(0);
}


static int cmd_date(struct cmd_funcparam *p) {
  struct nls_patterns *nls = (void *)(p->BUFFER);
  char *buff = p->BUFFER + (BUFFER_SIZE / 2);
  unsigned short i;
  unsigned short year = 0;
  unsigned char mo, dy;

  if (cmd_ishlp(p)) {
    outputnl("Displays or sets the system date.");
    outputnl("");
    outputnl("DATE [date]");
    outputnl("");
    outputnl("Type DATE with no parameters to display the current date and a prompt for a");
    outputnl("new one. Press ENTER to keep the same date.");
    return(-1);
  }

  i = nls_getpatterns(nls);
  if (i != 0) {
    outputnl(doserr(i));
    return(-1);
  }

  /* display current date if no args */
  if (p->argc == 0) {
    /* get cur date */
    _asm {
      push ax
      push cx
      push dx

      mov ah, 0x2a  /* DOS 1+ -- Query DOS Date */
      int 0x21      /* CX=year DH=month DL=day */
      mov [year], cx
      mov [mo], dh
      mov [dy], dl

      pop dx
      pop cx
      pop ax
    }
    buff[0] = ' ';
    nls_format_date(buff + 1, year, mo, dy, nls);
    output("Current date is");
    outputnl(buff);
    year = 0;
  } else { /* parse date if provided */
    if ((cmd_date_parse(p->argv[0], &year, &mo, &dy, nls) != 0) || (cmd_date_set(year, mo, dy) != 0)) {
      outputnl("Invalid date");
      year = 0;
    }
  }

  /* ask for date if not provided or if input was malformed */
  while (year == 0) {
    output("Enter new date:");
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
    if (buff[1] == 0) break; /* empty string = no date change */
    if ((cmd_date_parse(buff + 2, &year, &mo, &dy, nls) == 0) && (cmd_date_set(year, mo, dy) == 0)) break;
    outputnl("Invalid date");
  }

  return(-1);
}

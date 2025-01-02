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
 * vol [drive:]
 */

static void cmd_vol_internal(unsigned char drv, char *buff) {
  unsigned short *buff16 = (void *)(buff);
  unsigned short err = 0;
  struct DTA *dta = (void *)0x80; /* use the default DTA at location 80h in PSP */

  outputnl("");  /* start with an empty line to mimic MS-DOS */

  /* look for volume label in root dir via a FindFirst call */
  sv_strcpy(buff, "A:\\????????.???");
  buff[0] += drv;

  _asm {
    push ax
    push cx
    push dx
    mov [err], 0    /* preset errflag to zero */
    mov ah, 0x4e  /* FindFirst */
    mov dx, buff
    mov cx, 0x08  /* match volume labels only */
    int 0x21      /* dta filled or CF set on error */
    jnc DONE
    mov [err], ax
    DONE:
    pop dx
    pop cx
    pop ax
  }

  if (err != 0) {
    sv_strcpy(buff, svarlang_str(34,2)); /* "Volume in drive @ has no label" */
    sv_strtr(buff, '@', 'A' + drv);
  } else {
    /* if label > 8 chars then drop the dot (DRIVE_LA.BEL -> DRIVE_LABEL) */
    if (sv_strlen(dta->fname) > 8) memcpy_ltr(dta->fname + 8, dta->fname + 9, 4);
    sv_strcpy(buff, svarlang_str(34,3)); /* "Volume in drive @ is %" */
    sv_strtr(buff, '@', 'A' + drv);
    sv_insert_str_in_str(buff, dta->fname);
  }
  outputnl(buff);

  /* try to fetch the disk's serial number (DOS 4+ internal call) */
  err = 0;
  _asm {
    push ax
    push bx
    push dx
    mov ax, 0x6900
    mov bl, drv  /* A=1, B=2, etc */
    inc bl       /* adjust BL to +1 since drv is 0-based (A=0, B=1, etc) */
    xor bh, bh   /* "info level", must be 0 */
    mov dx, buff /* pointer to a location where a DiskInfo struct will be written */
    int 0x21
    jnc DONE
    mov [err], ax  /* err code */
    DONE:
    pop dx
    pop bx
    pop ax
  }
  /* Format of DiskInfo struct (source: RBIL)
   Offset  Size    Description (Table 01766)
   00h     WORD    0000h (info level)
   02h     DWORD   disk serial number (binary)
   06h  11 BYTEs   volume label or "NO NAME    " if none present
   11h   8 BYTEs   filesystem type */
  if ((err == 0) && (buff16[1] | buff16[2])) {
    char serialnum[10];

    /* fill serialnum with... the disk's serial num */
    ustoh(serialnum, buff16[2]);
    serialnum[4] = '-';
    ustoh(serialnum+5, buff16[1]);

    sv_strcpy(buff + 6, svarlang_str(34,4)); /* "Volume Serial Number is %" */
    sv_insert_str_in_str(buff + 6, serialnum);
    outputnl(buff + 6);
  }
}


static enum cmd_result cmd_vol(struct cmd_funcparam *p) {
  char drv = 0;
  char curdrv = 0;
  unsigned short i;

  if (cmd_ishlp(p)) {
    nls_outputnl(34,0); /* "Displays the disk volume label and serial number, if they exist." */
    outputnl("");
    nls_outputnl(34,1); /* "VOL [drive:]" */
    return(CMD_OK);
  }

  for (i = 0; i < p->argc; i++) {
    if (p->argv[i][0] == '/') {
      nls_outputnl(0,2); /* "Invalid switch" */
      return(CMD_FAIL);
    }
    if (drv != 0) {
      nls_outputnl(0,4); /* "Too many parameters" */
      return(CMD_FAIL);
    }
    if ((p->argv[i][0] == 0) || (p->argv[i][1] != ':') || (p->argv[i][2] != 0)) {
      nls_outputnl(0,3); /* "Invalid parameter format" */
      return(CMD_FAIL);
    }
    drv = p->argv[i][0];
    /* convert drive letter to a value 1..x (1=A, 2=B, etc) */
    if ((drv >= 'a') && (drv <= 'z')) {
      drv -= 'a';
    } else {
      drv -= 'A';
    }
  }

  /* fetch current drive */
  _asm {
    push ax
    mov ah, 0x19  /* query default (current) disk */
    int 0x21      /* drive in AL (0=A, 1=B, etc) */
    mov [curdrv], al
    pop ax
  }

  /* if no drive specified, use the default one */
  if (drv == 0) {
    drv = curdrv;
  } else if (!isdrivevalid(drv)) { /* is specified drive valid? */
    nls_outputnl(255,15); /* "Invalid drive" */
    return(CMD_FAIL);
  }

  cmd_vol_internal(drv, p->BUFFER);

  return(CMD_OK);
}

/*
 * vol [drive:]
 */

static void cmd_vol_internal(unsigned char drv, char *buff) {
  unsigned short *buff16 = (void *)(buff);
  unsigned short err = 0;
  struct DTA *dta = (void *)0x80; /* use the default DTA at location 80h in PSP */

  outputnl("");  /* start with an empty line to mimic MS-DOS */

  /* look for volume label in root dir via a FindFirst call */
  sprintf(buff, "%c:\\????????.???", drv + 'A');
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
    sprintf(buff, "Volume in drive %c has no label", drv + 'A');
  } else {
    /* if label > 8 chars then drop the dot (DRIVE_LA.BEL -> DRIVE_LABEL) */
    if (strlen(dta->fname) > 8) memmove(dta->fname + 8, dta->fname + 9, 4);
    sprintf(buff, "Volume in drive %c is %s", drv + 'A', dta->fname);
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
    sprintf(buff + 64, "Volume Serial Number is %04X-%04X", buff16[2], buff16[1]);
    outputnl(buff + 64);
  }
}


static int cmd_vol(struct cmd_funcparam *p) {
  char drv = 0;
  char curdrv = 0;
  unsigned short i;

  if (cmd_ishlp(p)) {
    outputnl("Displays the disk volume label and serial number, if they exist.");
    outputnl("");
    outputnl("VOL [drive:]");
    return(-1);
  }

  for (i = 0; i < p->argc; i++) {
    if (p->argv[i][0] == '/') {
      outputnl("Invalid switch");
      return(-1);
    }
    if (drv != 0) {
      outputnl("Too many parameters");
      return(-1);
    }
    if ((p->argv[i][0] == 0) || (p->argv[i][1] != ':') || (p->argv[i][2] != 0)) {
      outputnl("Invalid parameter format");
      return(-1);
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
    outputnl("Invalid drive");
    return(-1);
  }

  cmd_vol_internal(drv, p->BUFFER);

  return(-1);
}

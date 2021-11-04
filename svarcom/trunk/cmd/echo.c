/*
 * echo
 */

static int cmd_echo(struct cmd_funcparam *p) {
  unsigned short offs = FP_OFF(p->cmdline) + 5;
  unsigned short segm = FP_SEG(p->cmdline);
  unsigned char far *echostatus = MK_FP(p->rmod_seg, RMOD_OFFSET_ECHOFLAG);

  /* display help only if /? is the only argument */
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    outputnl("Displays messages, or turns command-echoing on or off");
    outputnl("");
    outputnl("ECHO [ON | OFF]");
    outputnl("ECHO [message]");
    outputnl("");
    outputnl("Type ECHO without parameters to display the current echo setting.");
    return(-1);
  }

  /* ECHO without any parameter: display current state */
  if (p->argc == 0) {
    if (*echostatus) {
      outputnl("ECHO is on");
    } else {
      outputnl("ECHO is off");
    }
    return(-1);
  }

  /* ECHO ON */
  if ((p->argc == 1) && (imatch(p->argv[0], "on"))) {
    *echostatus = 1;
    return(-1);
  }

  /* ECHO OFF */
  if ((p->argc == 1) && (imatch(p->argv[0], "off"))) {
    *echostatus = 0;
    return(-1);
  }

  /* ECHO MSG (start at cmdline+5 since first 5 are "ECHO" + separator) */
  _asm {
    push ax
    push dx
    push ds
    push si

    mov si, [offs]
    cld           /* clear direction flag (DF) so lodsb increments SI */
    mov ah, 0x02  /* display char from DL */
    mov ds, [segm]
    NEXTYBTE:
    lodsb         /* load byte at DS:[SI] into AL and inc SI (if DF clear) */
    or al, al     /* is AL == 0? then end of string reached */
    jz DONE
    mov dl, al
    int 0x21
    jmp NEXTYBTE

    /* output a final CR/LF */
    DONE:
    mov dl, 0x0D
    int 0x21
    mov dl, 0x0A
    int 0x21

    pop si
    pop ds
    pop dx
    pop ax
  }

  return(-1);
}

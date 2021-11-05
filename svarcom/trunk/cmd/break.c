/*
 * break
 */

static int cmd_break(struct cmd_funcparam *p) {
  unsigned char brkflag = 0;

  if (cmd_ishlp(p)) {
    outputnl("Sets or clears extended CTRL+C checking");
    outputnl("");
    outputnl("BREAK [ON | OFF]");
    outputnl("");
    outputnl("Type BREAK without a parameter to display the current BREAK setting.");
    return(-1);
  }

  /* no params: display current break state */
  if (p->argc == 0) {
    _asm {
      push ax
      push dx

      mov ax, 0x3300   /* query break-check flag */
      int 0x21         /* status (0=OFF, 1=ON) in DL */
      mov [brkflag], dl

      pop dx
      pop ax
    }
    if (brkflag == 0) {
      outputnl("BREAK is off");
    } else {
      outputnl("BREAK is on");
    }
    return(-1);
  }

  /* too many params? */
  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(-1);
  }

  /* exactly 1 parameter - "on" or "off" */
  if (imatch(p->argv[0], "on")) {
    brkflag = 1;
  } else if (!imatch(p->argv[0], "off")) {
    outputnl("Invalid parameter");
    return(-1);
  }

  /* set break accordingly to brkflag */
  _asm {
    push ax
    push dx

    mov ax, 0x3301     /* set break-check level */
    mov dl, [brkflag]  /* 0=OFF 1=ON */
    int 0x21

    pop dx
    pop ax
  }

  return(-1);
}

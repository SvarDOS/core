/*
 * verify
 */

static int cmd_verify(struct cmd_funcparam *p) {

  if (cmd_ishlp(p)) {
    outputnl("Tells DOS whether to verify that files are written correctly to disk.");
    outputnl("\r\nVERIFY [ON | OFF]\r\n");
    outputnl("Type VERIFY without a parameter to display its current setting.");
    return(-1);
  }

  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(-1);
  }

  if (p->argc == 0) {
    unsigned char verstate = 0;
    _asm {
      push ax
      mov ah, 0x54   /* Get VERIFY status */
      int 0x21       /* AL == 0 (off) or AL == 1 (on) */
      mov [verstate], al
      pop ax
    }
    if (verstate == 0) {
      outputnl("VERIFY is off");
    } else {
      outputnl("VERIFY is on");
    }
    return(-1);
  }

  /* argc == 1*/
  if (imatch(p->argv[0], "on")) {
    _asm {
      push ax
      push dx
      mov ax, 0x2e01  /* set verify ON */
      xor dl, dl      /* apparently required by MS-DOS 2.x */
      int 0x21
      pop dx
      pop ax
    }
  } else if (imatch(p->argv[0], "off")) {
    _asm {
      push ax
      push dx
      mov ax, 0x2e00  /* set verify OFF */
      xor dl, dl      /* apparently required by MS-DOS 2.x */
      int 0x21
      pop dx
      pop ax
    }
  } else {
    outputnl("Must specify ON or OFF");
  }

  return(-1);
}

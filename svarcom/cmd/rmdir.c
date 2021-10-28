/*
 * rmdir
 */

static int cmd_rmdir(struct cmd_funcparam *p) {
  const char *dname = p->argv[0];
  unsigned short err = 0;

  if (cmd_ishlp(p)) {
    outputnl("Removes (deletes) a directory");
    outputnl("");
    outputnl("RMDIR [drive:]path");
    outputnl("RD [drive:]path");
    return(-1);
  }

  if (p->argc == 0) {
    outputnl("Required parameter missing");
    return(-1);
  }

  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(-1);
  }

  if (p->argv[0][0] == '/') {
    outputnl("Invalid parameter");
    return(-1);
  }

  _asm {
    push ax
    push dx

    mov ah, 0x3a   /* delete a directory, DS:DX points to ASCIIZ dir name */
    mov dx, [dname]
    int 0x21
    jnc DONE
    mov [err], ax
    DONE:

    pop dx
    pop ax
  }

  if (err != 0) outputnl(doserr(err));

  return(-1);
}
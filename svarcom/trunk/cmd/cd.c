/*
 * chdir
 *
 * displays the name of or changes the current directory.
 *
 * CHDIR [drive:][path]
 * CD..
 *
 * Type CD drive: to display the current directory in the specified drive.
 * Type CD without parameters to display the current drive and directory.
 */


static int cmd_cd(struct cmd_funcparam *p) {
  char *buffptr = p->BUFFER;

  /* CD /? */
  if (cmd_ishlp(p)) {
    outputnl("Displays the name of or changes the current directory.");
    outputnl("");
    outputnl("CHDIR [drive:][path]");
    outputnl("CHDIR[..]");
    outputnl("CD [drive:][path]");
    outputnl("CD[..]");
    outputnl("");
    outputnl(".. Specifies that you want to change to the parent directory.");
    outputnl("");
    outputnl("Type CD drive: to display the current directory in the specified drive.");
    outputnl("Type CD without parameters to display the current drive and directory.");
    return(-1);
  }

  /* one argument max */
  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(-1);
  }

  /* no argument? display current drive and dir ("CWD") */
  if (p->argc == 0) {
    curpathfordrv(buffptr, 0);
    outputnl(buffptr);
    return(-1);
  }

  /* argument can be either a drive (D:) or a path */
  if (p->argc == 1) {
    const char *arg = p->argv[0];
    unsigned short err = 0;
    /* drive (CD B:) */
    if ((arg[0] != '\\') && (arg[1] == ':') && (arg[2] == 0)) {
      unsigned char drive = arg[0];
      if (drive >= 'a') {
        drive -= ('a' - 1);
      } else {
        drive -= ('A' - 1);
      }

      err = curpathfordrv(buffptr, drive);
      if (err == 0) outputnl(buffptr);
    } else { /* path */
      _asm {
        push dx
        push ax
        mov ah, 0x3B  /* CHDIR (set current directory) */
        mov dx, arg
        int 0x21
        jnc DONE
        mov [err], ax
        DONE:
        pop ax
        pop dx
      }
    }
    if (err != 0) {
      outputnl(doserr(err));
    }
  }

  return(-1);
}

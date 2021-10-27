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


/* display current path drive d (A=1, B=2, etc)
 * returns 0 on success, doserr otherwise */
static unsigned short cmd_cd_curpathfordrv(char *buff, unsigned char d) {
  unsigned short r = 0;

  buff[0] = d + 'A' - 1;
  buff[1] = ':';
  buff[2] = '\\';

  _asm {
    push si
    push ax
    push dx
    mov ah, 0x47      /* get current directory */
    mov dl, [d]       /* A: = 1, B: = 2, etc */
    mov si, buff      /* append cur dir to buffer */
    add si, 3         /* skip the present drive:\ prefix */
    int 0x21
    jnc DONE
    mov [r], ax       /* copy result from ax */
    DONE:
    pop dx
    pop ax
    pop si
  }

  return(r);
}


static int cmd_cd(struct cmd_funcparam *p) {
  char *buffptr = p->BUFFER;

  /* two arguments max */
  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(-1);
  }

  /* CD /? */
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
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

  /* no argument? display current drive and dir ("CWD") */
  if (p->argc == 0) {
    unsigned char drv = 0;

    _asm {
      push ax
      push dx
      push si
      mov ah, 0x19  /* get current default drive */
      int 0x21      /* al = drive (00h = A:, 01h = B:, etc) */
      inc al        /* convert to 1=A, 2=B, etc */
      mov [drv], al
    }

    cmd_cd_curpathfordrv(buffptr, drv);
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

      err = cmd_cd_curpathfordrv(buffptr, drive);
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

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

static int cmd_cd(const struct cmd_funcparam *p) {
  /* two arguments max */
  if (p->argc > 1) {
    puts("Too many parameters");
  }

  /* no argument? display current drive and dir ("CWD") */
  if (p->argc == 0) {
    char buff[64];
    char *buffptr = buff;
    _asm {
      push ax
      push dx
      push si
      mov ah, 0x19  /* get current default drive */
      int 0x21      /* al = drive (00h = A:, 01h = B:, etc) */
      add al, 'A'
      /* print drive to stdout */
      mov dl, al
      mov ah, 0x02
      int 0x21
      mov dl, ':'
      int 0x21
      mov dl, '\'
      int 0x21
      /* get current dir */
      mov ah, 0x47
      xor dl, dl       /* select drive (0 = current drive) */
      mov si, buffptr  /* 64-byte buffer for ASCIZ pathname */
      int 0x21
      pop si
      pop dx
      pop ax
    }
    puts(buff);
  }

  /* argument can be either a drive (D:) or a path */
  if (p->argc == 1) {
    const char *arg = p->argv[0];
    unsigned short err = 0;
    /* drive (CD B:) */
    if ((arg[0] != '\\') && (arg[1] == ':') && (arg[2] == 0)) {
      char buff[64];
      char *buffptr = buff;
      unsigned char drive = arg[0];
      if (drive >= 'a') {
        drive -= 'a';
      } else {
        drive -= 'A';
      }
      drive++; /* A: = 1, B: = 2, etc*/
      _asm {
        push si
        push ax
        push dx
        mov ah, 0x47      /* get current directory */
        mov dl, [drive]   /* A: = 1, B: = 2, etc */
        mov si, buffptr
        int 0x21
        jnc DONE
        mov [err], ax
        DONE:
        pop dx
        pop ax
        pop si
      }
      if (err == 0) printf("%c:\\%s\r\n", drive + 'A' - 1, buff);
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
    if (err != 0) puts(doserr(err));
  }

  return(-1);
}

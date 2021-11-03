
#include "doserr.h"
#include "helpers.h"

#include "redir.h"

static unsigned short oldstdout = 0xffff;

/* parse commandline and performs necessary redirections. cmdline is
 * modified so all redirections are cut out.
 * returns 0 on success, non-zero otherwise */
int redir_parsecmd(char far *cmdline, char *BUFFER) {
  unsigned short i;
  unsigned short rediroffset_stdin = 0;
  unsigned short rediroffset_stdout = 0;
  unsigned short pipesoffsets[16];
  unsigned short pipescount = 0;

  /* NOTE:
   *
   * 1. while it is possible to type a command with multiple
   *    redirections, MSDOS executes only the last redirection.
   *
   * 2. the order in which >, < and | are provided on the command line does
   *    not seem to matter for MSDOS. piped commands are executed first (in
   *    the order they appear) and the result of the last one is redirected to
   *    whenever the last > points at.
   */

  /* preset oldstdout to 0xffff in case no redirection is required */
  oldstdout = 0xffff;

  for (i = 0;; i++) {
    if (cmdline[i] == '>') {
      cmdline[i] = 0;
      rediroffset_stdout = i + 1;
    } else if (cmdline[i] == '<') {
      cmdline[i] = 0;
      rediroffset_stdin = i + 1;
    } else if (cmdline[i] == '|') {
      cmdline[i] = 0;
      pipesoffsets[pipescount++] = i + 1;
    } else if (cmdline[i] == 0) {
      break;
    }
  }

  if (rediroffset_stdin != 0) {
    outputnl("ERROR: stdin redirection is not supported yet");
    return(-1);
  }

  if (pipescount != 0) {
    outputnl("ERROR: pipe redirections are not supported yet");
    return(-1);
  }

  if (rediroffset_stdout != 0) {
    unsigned short openflag = 0x12;  /* used during the int 21h,ah=6c call */
    unsigned short errcode = 0;
    unsigned short handle = 0;
    char far *ptr;
    /* append? */
    if (cmdline[rediroffset_stdout] == '>') {
      openflag = 0x11;
      rediroffset_stdout++;
    }

    /* copy dst file to BUFFER */
    ptr = cmdline + rediroffset_stdout;
    while (*ptr == ' ') ptr++; /* skip leading white spaces */
    for (i = 0;; i++) {
      BUFFER[i] = ptr[i];
      if ((BUFFER[i] == ' ') || (BUFFER[i] == 0)) break;
    }
    BUFFER[i] = 0;

    /* */
    _asm {
      push ax
      push bx
      push cx
      push dx
      push si
      mov ax, 0x6c00     /* Extended Open/Create */
      mov bx, 1          /* access mode (0=read, 1=write, 2=r+w */
      xor cx, cx         /* attributes when(if) creating the file (0=normal) */
      mov dx, [openflag] /* action if file exists (0x11=open, 0x12=truncate)*/
      mov si, BUFFER     /* ASCIIZ filename */
      int 0x21           /* AX=handle on success (CF clear), otherwise dos err */
      mov [handle], ax   /* save the file handler */
      jnc DUPSTDOUT
      mov [errcode], ax
      jmp DONE
      DUPSTDOUT:
      /* save (duplicate) current stdout so I can revert it later */
      mov ah, 0x45       /* duplicate file handle */
      mov bx, 1          /* handle to dup (1=stdout) */
      int 0x21           /* ax = new file handle */
      mov [oldstdout], ax
      /* redirect the stdout handle */
      mov bx, [handle]   /* dst handle */
      mov cx, 1          /* src handle (1=stdout) */
      mov ah, 0x46       /* redirect a handle */
      int 0x21
      /* close the original file handle (no longer needed) */
      mov ah, 0x3e       /* close a file handle (handle in BX) */
      int 0x21
      DONE:
      pop si
      pop dx
      pop cx
      pop bx
      pop ax
    }
    if (errcode != 0) {
      outputnl(doserr(errcode));
      return(-1);
    }
  }

  return(0);
}


/* restores previous stdout/stdin handlers if they have been redirected */
void redir_revert(void) {
  _asm {
    /* if oldstdout is 0xffff then not redirected */
    cmp word ptr [oldstdout], 0xffff
    je DONE
    /* redirect the stdout handle */
    mov bx, [oldstdout] /* dst handle */
    mov cx, 1           /* src handle (1=stdout) */
    mov ah, 0x46        /* redirect a handle */
    int 0x21
    mov [oldstdout], 0xffff
    DONE:
  }
}

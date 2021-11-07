/*
 * chcp
 */

static int cmd_chcp(struct cmd_funcparam *p) {
  unsigned short nnn = 0;
  unsigned short errcode = 0;

  if (cmd_ishlp(p)) {
    outputnl("Displays or sets the active code page number");
    outputnl("");
    outputnl("CHCP [nnn]");
    outputnl("");
    outputnl("nnn  Specifies a code page number");
    outputnl("");
    outputnl("Type CHCP without a parameter to display the active code page number.");
    return(-1);
  }

  /* too many parameters */
  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(-1);
  }

  /* one param? must be numeric in range 1+ */
  if (p->argc == 1) {
    unsigned char nlsfuncflag = 0;
    if (atouns(&nnn, p->argv[0]) != 0) {
      outputnl("Invalid code page number");
      return(-1);
    }
    _asm {
      /* verify that NLSFUNC is installed */
      push ax
      push bx

      mov ax, 0x1400    /* DOS 3+ -- is NLSFUNC.EXE installed? */
      int 0x2f          /* AL = 0xff -> installed */
      cmp al, 0xff
      jne DONE
      mov [nlsfuncflag], 1

      /* set code page to nnn */

      mov ax, 0x6602    /* DOS 3.3+ -- Activate Code Page */
      mov bx, [nnn]
      int 0x21          /* CF set on error and err code in AX */
      jnc DONE
      mov [errcode], ax /* store err code in nnn on failure */
      DONE:

      pop bx
      pop ax
    }
    if (nlsfuncflag == 0) {
      outputnl("NLSFUNC not installed");
    } else if (errcode != 0) {
      outputnl("Failed to change code page");
    }

  } else { /* no parameter given: display active code page */

    _asm {
      push ax
      push bx
      push dx

      mov ax, 0x6601      /* DOS 3.3+ -- Query Active Code Page */
      int 0x21            /* CF set on error, current CP in BX */
      mov [nnn], bx
      jnc DONE
      mov [errcode], ax
      DONE:

      pop dx
      pop bx
      pop ax
    }
    if (errcode == 0) {
      sprintf(p->BUFFER, "Active code page: %d", nnn);
      outputnl(p->BUFFER);
    } else {
      outputnl(doserr(errcode));
    }
  }

  return(-1);
}

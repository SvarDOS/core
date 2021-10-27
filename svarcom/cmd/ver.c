/*
 * ver
 */

#define PVER "20211027"

static int cmd_ver(struct cmd_funcparam *p) {
  char *buff = p->BUFFER;
  unsigned char maj = 0, min = 0;

  /* help screen */
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    outputnl("Displays the DOS version.");
    return(-1);
  }

  if (p->argc != 0) {
    outputnl("Invalid parameter");
    return(-1);
  }

  _asm {
    push ax
    push bx
    push cx
    mov ah, 0x30   /* Get DOS version number */
    int 0x21       /* AL=maj_ver_num  AH=min_ver_num  BX,CX=OEM */
    mov [maj], al
    mov [min], ah
    pop cx
    pop bx
    pop ax
  }

  sprintf(buff, "DOS kernel version %u.%u", maj, min);

  outputnl(buff);
  outputnl("SvarCOM shell ver " PVER);
  return(-1);
}

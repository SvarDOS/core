/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * ver
 */

#define PVER "2021.0"
#define COPYRDATE "2021"

static int cmd_ver(struct cmd_funcparam *p) {
  char *buff = p->BUFFER;
  unsigned char maj = 0, min = 0;

  /* help screen */
  if (cmd_ishlp(p)) {
    outputnl("Displays the DOS version.");
    outputnl("");
    outputnl("ver [/about]");
    return(-1);
  }

#if 1
  if ((p->argc == 1) && (imatch(p->argv[0], "/dbg"))) {
    unsigned short far *rmod_envseg = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_ENVSEG);
    unsigned char far *rmod_exitcode = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_LEXITCODE);
    unsigned short far *rmod_comspecptr = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_COMSPECPTR);
    char far *fptr;
    unsigned short i;
    printf("rmod->rmodseg = 0x%04X\r\n", p->rmod->rmodseg);
    printf("rmod->origparent = %04X:%04X\r\n", p->rmod->origparent >> 16, p->rmod->origparent & 0xffff);
    printf("rmod->origenvseg = 0x%04X\r\n", p->rmod->origenvseg);
    printf("rmod->flags = 0x%02X\r\n", p->rmod->flags);
    printf("rmod->echoflag = %u\r\n", p->rmod->echoflag);
    printf("[rmod:RMOD_OFFSET_ENVSEG] = 0x%04X\r\n", *rmod_envseg);
    printf("environment allocated size: %u bytes\r\n", env_allocsz(*rmod_envseg));
    for (fptr = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_BOOTDRIVE), i = 0; *fptr != 0; fptr++) buff[i++] = *fptr;
    buff[i] = 0;
    printf("[rmod:RMOD_OFFSET_BOOTCOMSPEC] = '%s'\r\n", buff);
    if (*rmod_comspecptr == 0) {
      sprintf(buff, "NULL");
    } else {
      for (fptr = MK_FP(*rmod_envseg, *rmod_comspecptr), i = 0; *fptr != 0; fptr++) buff[i++] = *fptr;
      buff[i] = 0;
    }
    printf("[rmod:RMOD_OFFSET_COMSPECPTR] = '%s'\r\n", buff);
    printf("[rmod:RMOD_OFFSET_LEXITCODE] = %u\r\n", *rmod_exitcode);
    return(-1);
  }
#endif

  if ((p->argc == 1) && (imatch(p->argv[0], "/about"))) {
    outputnl("SvarCOM is a shell interpreter for DOS kernels compatible with MS-DOS 5+.");
    outputnl("");
    outputnl("This software is distributed under the terms of the MIT license.");
    outputnl("Copyright (C) " COPYRDATE " Mateusz Viste");
    outputnl("");
    outputnl("Prace te dedykuje Milenie i Mojmirowi. Zycze wam, abyscie w dalszym zyciu");
    outputnl("potrafili wlasciwie docenic wartosc czasow minionych i czerpali radosc z");
    outputnl("prostych przyjemnosci dnia codziennego.  Lair, jesien 2021.");
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

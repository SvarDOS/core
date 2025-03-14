/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2024 Mateusz Viste
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


/* takes two numbers and builds a string of the format "%u.%02u" */
static void cmd_ver_buildver(char *buf, unsigned char x, unsigned char y) {
  buf += ustoa(buf, x, 0, '0');
  *buf = '.';
  buf++;
  ustoa(buf, y, 2, '0');
}


static enum cmd_result cmd_ver(struct cmd_funcparam *p) {
  char *buff = p->BUFFER;
  unsigned char maj = 0, min = 0, retcode = 0, truemaj = 0, truemin = 0, rev = 0, verflags = 0;
  unsigned short drdosver = 0;
  unsigned short doscrelstrseg = 0, doscrelstroff = 0;

  /* help screen */
  if (cmd_ishlp(p)) {
    nls_outputnl(20,0); /* "Displays the DOS kernel and SvarCOM shell versions." */
    outputnl("");
    output("ver [/about]");
#ifdef VERDBG
    output(" [/dbg]");
#endif
    outputnl("");
    return(CMD_OK);
  }

#ifdef VERDBG
  if ((p->argc == 1) && (imatch(p->argv[0], "/dbg"))) {
    unsigned short far *rmod_envseg = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_ENVSEG);
    unsigned char far *rmod_exitcode = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_LEXITCODE);
    unsigned short far *rmod_comspecptr = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_COMSPECPTR);
    char far *fptr;
    unsigned short i;
    printf("rmod->rmodseg = 0x%04X\r\n", p->rmod->rmodseg);
    printf("rmod->origint22 = %04X:%04X\r\n", p->rmod->origint22 >> 16, p->rmod->origint22 & 0xffff);
    printf("rmod->origparent = 0x%04X\r\n", p->rmod->origparent);
    printf("rmod->flags = 0x%02X\r\n", p->rmod->flags);
    printf("[rmod:RMOD_OFFSET_ENVSEG] = 0x%04X\r\n", *rmod_envseg);
    printf("environment allocated size: %u bytes\r\n", env_allocsz(*rmod_envseg));
    for (fptr = MK_FP(p->rmod->rmodseg, RMOD_OFFSET_BOOTDRIVE), i = 0; *fptr != 0; fptr++) buff[i++] = *fptr;
    buff[i] = 0;
    printf("[rmod:RMOD_OFFSET_BOOTCOMSPEC] = '%s'\r\n", buff);
    if (*rmod_comspecptr == 0) {
      sv_strcpy(buff, "NULL");
    } else {
      for (fptr = MK_FP(*rmod_envseg, *rmod_comspecptr), i = 0; *fptr != 0; fptr++) buff[i++] = *fptr;
      buff[i] = 0;
    }
    printf("[rmod:RMOD_OFFSET_COMSPECPTR] = '%s'\r\n", buff);
    printf("[rmod:RMOD_OFFSET_LEXITCODE] = %u\r\n", *rmod_exitcode);
    printf("rmod dump (first 64 bytes at [rmodseg:0100h]):\r\n");
    fptr = MK_FP(p->rmod->rmodseg, 0x100);
    for (i = 0; i < 64; i += 16) {
      int ii;
      for (ii = i; ii < i + 16; ii++) printf(" %02X", fptr[ii]);
      printf("   ");
      for (ii = i; ii < i + 16; ii++) {
        if (fptr[ii] < ' ') {
          printf(".");
        } else {
          printf("%c", fptr[ii]);
        }
      }
      printf("\r\n");
    }

    return(CMD_OK);
  }
#endif

  if ((p->argc == 1) && (imatch(p->argv[0], "/about"))) {
    nls_outputnl(20,3); /* "SvarCOM is a shell interpreter for DOS kernels compatible with MS-DOS 5+." */
    outputnl("");
    nls_outputnl(20,4); /* "This software is distributed under the terms of the MIT license." */
    outputnl("Copyright (C) " COPYRDATE " Mateusz Viste");
    outputnl("");
    outputnl("Program ten dedykuje Milenie i Mojmirowi. Zycze wam, byscie w swoim zyciu");
    outputnl("potrafili docenic wartosci minionych pokolen, jednoczesnie czerpiac radosc");
    outputnl("z prostych przyjemnosci dnia codziennego.  Lair, jesien 2021.");
    return(CMD_OK);
  }

  _asm {
    push ax
    push bx
    push cx
    push dx

    /* get the "normal" (spoofable) DOS version */
    mov ah, 0x30  /* function supported on DOS 2+ */
    int 0x21      /* AL=maj_ver_num  AH=min_ver_num  BX,CX=OEM */
    mov [maj], al
    mov [min], ah

    /* get the "true" DOS version, along with a couple of extra data */
    mov ax, 0x3306     /* function supported on DOS 5+ */
    int 0x21
    mov [retcode], al  /* AL=return_code for DOS < 5 */
    mov [truemaj], bl  /* BL=maj_ver_num  BH=min_ver_num */
    mov [truemin], bh
    mov [rev], dl      /* DL=revision  DH=kernel_memory_area */
    mov [verflags], dh

    /* get the DR-DOS specific version through INT 21h,4452h */
    mov ax, 0x4452
    int 0x21
    jc NOTDRDOS
    mov drdosver, ax
    NOTDRDOS:

    /* get a pointer to the DOS-C style "os_release" via INT 21h,33FFh */
    mov ax, 0x33ff
    xor dx, dx
    int 0x21
    jc NOTDOSC
    mov doscrelstrseg, dx
    mov doscrelstroff, ax
    NOTDOSC:

    pop dx
    pop cx
    pop bx
    pop ax
  }

  /* look for a DOS-C os_release (FreeDOS, EDR-DOS, RX-DOS...)
   * https://github.com/SvarDOS/edrdos/issues/77 */
  if (doscrelstrseg != 0) {
    char far *osrel = MK_FP(doscrelstrseg, doscrelstroff);
    unsigned short i;
    for (i = 0; osrel[i] != 0; i++) {
      buff[i] = osrel[i];
    }
    buff[i] = 0;
    outputnl(buff);
  }

  if (drdosver != 0) {
    char hexbuf[8];
    sv_strcpy(buff, svarlang_str(20,11)); /* DR-DOS kernel version % */
    ustoh(hexbuf, drdosver);
    sv_insert_str_in_str(buff, hexbuf);
    outputnl(buff);
  }

  {
  char verbuf[8];
  cmd_ver_buildver(verbuf, maj, min);
  sv_strcpy(buff, svarlang_str(20,1)); /* "DOS kernel version %" */
  sv_insert_str_in_str(buff, verbuf);
  output(buff);
  }

  /* can we trust in the data returned? */
  /* DR DOS 5&6 return 0x01, MS-DOS 2-4 return 0xff */
  /* 'truemaj' is checked to mitigate the conflict from the CBIS redirector */
  if ((retcode > 1) && (retcode < 255) && (truemaj > 4) && (truemaj < 100)) {
    if ((maj != truemaj) || (min != truemin)) {
      char verbuf[8];
      cmd_ver_buildver(verbuf, truemaj, truemin);
      output(" (");
      sv_strcpy(buff, svarlang_str(20,10)); /* "true ver %" */
      sv_insert_str_in_str(buff, verbuf);
      output(buff);
      output(")");
    }
    outputnl("");

    sv_strcpy(buff, svarlang_str(20,5)); /* "Revision %c" */
    sv_strtr(buff, '@', 'A' + rev);
    outputnl(buff);

    {
      const char *loc = svarlang_str(20,7);        /* "low memory" */
      if (verflags & 16) loc = svarlang_str(20,8); /* "HMA" */
      if (verflags & 8) loc = svarlang_str(20,9);  /* "ROM" */
      sv_strcpy(buff, svarlang_str(20,6));         /* "DOS is in %" */
      sv_insert_str_in_str(buff, loc);
      outputnl(buff);
    }
  }

  outputnl("");
  nls_output(20,2); /* "SvarCOM shell ver" */
  outputnl(" " PVER);

  return(CMD_OK);
}

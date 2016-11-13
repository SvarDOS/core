/*
 * input keyboard routines used by the Svarog386 installer.
 * Copyright (C) 2016 Mateusz Viste
 */

#ifndef input_h_sentinel
#define input_h_sentinel

#include <dos.h>
#include "input.h" /* include self for control */

/* waits for a keypress and return it. Returns 0x1xx for extended keys */
int input_getkey(void) {
  union REGS regs;
  int res;
  regs.h.ah = 0x08;
  int86(0x21, &regs, &regs);
  res = regs.h.al;
  if (res == 0) { /* extended key - poll again */
    regs.h.ah = 0x08;
    int86(0x21, &regs, &regs);
    res = regs.h.al | 0x100;
  }
  return(res);
}


/* poll the keyboard, and return the next input key in buffer, or -1 if none */
int input_getkeyifany(void) {
  union REGS regs;
  regs.h.ah = 0x0B;
  int86(0x21, &regs, &regs);
  if (regs.h.al == 0xFF) return(input_getkey());
  return(-1);
}

#endif

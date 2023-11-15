/*
 * replacement for a couple of libc functions
 */

#include <i86.h>
#include <stddef.h>

#include "libc.h"


unsigned short mdr_dos_write(unsigned short handle, const void far *buf, unsigned short count, unsigned short *bytes) {
  unsigned short res = 0;
  unsigned short resax = 0;
  unsigned short buf_seg = FP_SEG(buf);
  unsigned short buf_off = FP_OFF(buf);

  _asm {
    push bx
    push cx
    push dx

    mov ah, 0x40
    mov bx, handle
    mov cx, count
    mov dx, buf_off
    push ds
    mov ds, buf_seg

    int 0x21
    pop ds
    jnc done
    mov res, ax

    done:
    mov resax, ax

    pop dx
    pop cx
    pop bx
  }

  *bytes = resax;
  return(res);
}

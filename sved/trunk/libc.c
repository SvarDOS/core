/*
 * replacement for a couple of libc functions
 */

#include <i86.h>
#include <stddef.h>

#include "libc.h"


size_t strlen(const char *s) {
  size_t res = 0;
  while (s[res] != 0) res++;
  return(res);
}

void bzero(void *ptr, size_t len) {
  while (len > 0) ((char *)ptr)[--len] = 0;
}

/* TODO this function does not handle overlapping strings well! */
void far *_fmemmove(void far *dst, const void far *src, size_t len) {
  while (len-- > 0) {
    ((char far *)dst)[len] = ((char far *)src)[len];
  }
  return(dst);
}

unsigned short mdr_dos_fopen(const char *fname, unsigned short *fhandle) {
  unsigned short res = 0;
  unsigned short handle = 0;
  unsigned short fname_seg = FP_SEG(fname);
  unsigned short fname_off = FP_OFF(fname);
  _asm {
    push cx
    push dx

    mov ax, 0x3d00
    mov dx, fname_off
    xor cl, cl
    push ds
    mov ds, fname_seg
    int 0x21
    pop ds
    jc err
    mov handle, ax
    jmp done
    err:
    mov res, ax
    done:

    pop dx
    pop cx
  }
  *fhandle = handle;
  return(res);
}


unsigned short mdr_dos_fclose(unsigned short handle) {
  unsigned short res = 0;
  _asm {
    push bx

    mov ah, 0x3e
    mov bx, handle
    int 0x21
    jnc done
    mov res, ax
    done:

    pop bx
  }
  return(res);
}


unsigned short _dos_freemem(unsigned short segn) {
  unsigned short res = 0;
  _asm {
    push es
    mov ah, 0x49
    mov es, segn
    int 0x21
    pop es
    jnc done
    mov res, ax
    done:
  }
  return(res);
}


unsigned short mdr_dos_allocmem(unsigned short siz) {
  unsigned short segnum = 0;

  _asm {
    push bx

    mov ah, 0x48
    mov bx, siz
    int 0x21
    jc done
    mov segnum, ax

    done:

    pop bx
  }

  return(segnum);
}


unsigned short mdr_dos_resizeblock(unsigned short siz, unsigned short segn, unsigned short *maxsiz) {
  unsigned short resbx = 0;
  unsigned short res = 0;

  _asm {
    push bx
    push es

    mov ah, 0x4a
    mov bx, siz
    mov es, segn
    int 0x21
    jnc done
    mov resbx, bx
    mov res, ax

    done:

    pop es
    pop bx
  }

  *maxsiz = resbx;
  return(res);
}


unsigned short mdr_dos_read(unsigned short handle, void far *buf, unsigned short count, unsigned short *bytes) {
  unsigned short res = 0;
  unsigned short resax = 0;
  unsigned short buf_off = FP_OFF(buf);
  unsigned short buf_seg = FP_SEG(buf);

  _asm {
    push bx,
    push cx
    push dx

    mov ah, 0x3f
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

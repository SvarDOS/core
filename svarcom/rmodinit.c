
#include <i86.h>
#include <stdio.h>
#include <string.h>

#include "rmod.h"

#include "rmodinit.h"


/* returns segment where rmod is installed */
unsigned short rmod_install(unsigned short envsize) {
  char far *myptr, far *mcb;
  unsigned short far *owner;
  unsigned int rmodseg = 0xffff;
  unsigned int envseg = 0;

  /* read my current env segment from PSP */
  _asm {
    push ax
    push bx
    mov bx, 0x2c
    mov ax, [bx]
    mov envseg, ax
    pop bx
    pop ax
  }

  printf("original (PSP) env buffer at %04X\r\n", envseg);
  /* if custom envsize requested, convert it to number of paragraphs */
  if (envsize != 0) {
    envsize += 15;
    envsize /= 16;
  }


  _asm {
    /* link in the UMB memory chain for enabling high-memory allocation (and save initial status on stack) */
    mov ax, 0x5802  /* GET UMB LINK STATE */
    int 0x21
    xor ah, ah
    push ax         /* save link state on stack */
    mov ax, 0x5803  /* SET UMB LINK STATE */
    mov bx, 1
    int 0x21
    /* get current allocation strategy and save it in DX */
    mov ax, 0x5800
    int 0x21
    push ax
    pop dx
    /* set strategy to 'last fit, try high then low memory' */
    mov ax, 0x5801
    mov bx, 0x0082
    int 0x21
    /* ask for a memory block and save the given segment to rmodseg */
    mov ah, 0x48
    mov bx, (rmod_len + 15) / 16
    int 0x21
    jc ALLOC_FAIL
    mov rmodseg, ax
    /* ask for a memory block for the environment and save it to envseg (only if custom size requested) */
    mov bx, envsize
    test bx, bx
    jz ALLOC_FAIL
    mov ah, 0x48
    int 0x21
    jc ALLOC_FAIL
    mov envseg, ax

    ALLOC_FAIL:
    /* restore initial allocation strategy */
    mov ax, 0x5801
    mov bx, dx
    int 0x21
    /* restore initial UMB memory link state */
    mov ax, 0x5803
    pop bx       /* pop initial UMB link state from stack */
    int 0x21
  }

  if (rmodseg == 0xffff) {
    puts("malloc error");
    return(0xffff);
  }

  /* copy rmod to its destination */
  myptr = MK_FP(rmodseg, 0);
  _fmemcpy(myptr, rmod, rmod_len);

  /* mark rmod memory as "self owned" */
  mcb = MK_FP(rmodseg - 1, 0);
  owner = (void far *)(mcb + 1);
  *owner = rmodseg;
  _fmemcpy(mcb + 8, "SVARCOM", 8);

  /* mark env memory as "self owned" */
  mcb = MK_FP(envseg - 1, 0);
  owner = (void far *)(mcb + 1);
  *owner = rmodseg;
  _fmemcpy(mcb + 8, "SVARENV", 8);

  /* write env segment to rmod buffer */
  owner = MK_FP(rmodseg, RMOD_OFFSET_ENVSEG);
  *owner = envseg;

  /* write boot drive to rmod bootdrive field */
  _asm {
    push ax
    push bx
    push dx
    push ds
    mov ax, 0x3305 /* DOS 4.0+ - GET BOOT DRIVE */
    int 0x21 /* boot drive is in DL now (1=A:, 2=B:, etc) */
    add dl, 'A'-1 /* convert to a proper ASCII letter */
    /* set DS to rmodseg */
    mov ax, rmodseg
    mov ds, ax
    /* write boot drive to rmod bootdrive field */
    mov bx, RMOD_OFFSET_BOOTDRIVE
    mov [bx], dl
    pop ds
    pop dx
    pop bx
    pop ax
  }

  /* set the int22 handler in my PSP to rmod so DOS jumps to rmod after I terminate */
  _asm {
    push ax
    push bx
    mov bx, 0x0a                   /* int22 handler is at 0x0A of the PSP */
    mov ax, RMOD_OFFSET_ROUTINE
    mov [bx], ax                   /* int handler offset */
    mov ax, rmodseg
    mov [bx+2], ax                 /* int handler segment */
    pop bx
    pop ax
  }

  return(rmodseg);
}


/* scan memory for rmod, returns its segment if found, 0xffff otherwise */
unsigned short rmod_find(void) {
  unsigned short i;
  unsigned short far *ptrword;
  unsigned char far *ptrbyte;

  /* iterate over all paragraphs, looking for my signature */
  for (i = 1; i != 65535; i++) {
    ptrword = MK_FP(i, 0);
    if (ptrword[0] != 0x1983) continue;
    if (ptrword[1] != 0x1985) continue;
    if (ptrword[2] != 0x2017) continue;
    if (ptrword[3] != 0x2019) continue;
    /* extra check: make sure the paragraph before is an MCB block and that it
     * belongs to itself. otherwise I could find the rmod code embedded inside
     * the command.com binary... */
    ptrbyte = MK_FP(i - 1, 0);
    if ((*ptrbyte != 'M') && (*ptrbyte != 'Z')) continue; /* not an MCB */
    ptrword = MK_FP(i - 1, 1);
    if (*ptrword != i) continue; /* not belonging to self */
    return(i);
  }
  return(0xffff);
}

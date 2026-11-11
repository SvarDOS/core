/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2026 Mateusz Viste
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
 * ctty device
 *
 * Switches the console (stdin, stdout and stderr) of this shell and of RMOD.
 * External programs are executed by RMOD, which then reloads COMMAND.COM, so
 * a change made only in the transient PSP would be lost at the next program.
 *
 * Ordinary INT 21h calls record SS:SP in the current PSP. The calls issued
 * while RMOD is the current PSP run on RMOD's own stack, otherwise that
 * recorded pointer would point into the transient and the next exec would
 * corrupt it (bugz #178). AH=50h and AH=62h do not record a stack.
 */

/* DS-relative scratch used while SS points at RMOD's stack. BP-relative
 * locals are unreachable then, because BP is relative to SS. */
static unsigned short ctty_saved_ss;
static unsigned short ctty_saved_sp;
static unsigned short ctty_saved_psp;
static unsigned short ctty_saved_dev;
static unsigned short ctty_saved_handle;
static unsigned short ctty_rmod_err;


/* open dev read/write and require a character device. on success the handle
 * is left open and stored in *handle; it is not installed as stdio yet. */
static unsigned short ctty_open_device(const char *dev, unsigned short *handle) {
  unsigned short errcode = 0;
  unsigned short h = 0xffff;

  _asm {
    push ax
    push bx
    push dx

    mov ax, 0x3d02       /* open for read/write */
    mov dx, [dev]
    int 0x21
    jnc OPENED
    mov [errcode], ax
    jmp DONE

    OPENED:
    mov [h], ax
    mov bx, ax
    mov ax, 0x4400       /* IOCTL: get device information */
    int 0x21             /* DX bit 7 set when the handle is a device */
    jc NOTDEV
    test dl, 0x80
    jnz DONE

    NOTDEV:
    mov bx, [h]
    mov ah, 0x3e
    int 0x21
    mov [h], 0xffff
    mov [errcode], 1

    DONE:
    pop dx
    pop bx
    pop ax
  }

  *handle = h;
  return(errcode);
}


/* install dev as stdin/stdout/stderr of RMOD. the transient handles are not
 * changed. returns 0 on success. */
static unsigned short ctty_apply_rmod(const char *dev, unsigned short rmodseg) {
  unsigned short rmodsp = RMOD_OFFSET_STACKPTR;

  ctty_rmod_err = 0;

  _asm {
    push ax
    push bx
    push cx
    push dx
    push si
    push di

    mov ah, 0x62         /* get current PSP (does not record SS:SP) */
    int 0x21
    mov [ctty_saved_psp], bx

    mov dx, [dev]
    mov [ctty_saved_dev], dx

    mov ax, ss
    mov [ctty_saved_ss], ax
    mov ax, sp
    mov [ctty_saved_sp], ax

    mov ah, 0x50         /* set PSP to RMOD (does not record SS:SP) */
    mov bx, [rmodseg]
    int 0x21

    /* further INT 21h calls must run on RMOD's stack so DOS records that
     * stack in RMOD's PSP */
    mov ax, [rmodseg]
    mov bx, [rmodsp]
    cli
    mov ss, ax
    mov sp, bx
    sti

    mov dx, [ctty_saved_dev]
    mov ax, 0x3d02
    int 0x21
    jc RMOD_OPEN_FAIL
    mov [ctty_saved_handle], ax

    /* AH=46h: BX = existing handle, CX = handle to replace */
    mov bx, [ctty_saved_handle]
    xor cx, cx           /* stdin */
    mov ah, 0x46
    int 0x21
    jc RMOD_DUP_FAIL

    mov bx, [ctty_saved_handle]
    mov cx, 1            /* stdout */
    mov ah, 0x46
    int 0x21
    jc RMOD_DUP_FAIL

    mov bx, [ctty_saved_handle]
    mov cx, 2            /* stderr */
    mov ah, 0x46
    int 0x21
    jc RMOD_DUP_FAIL

    mov bx, [ctty_saved_handle]
    mov ah, 0x3e
    int 0x21
    jmp RMOD_STACK_BACK

    RMOD_DUP_FAIL:
    mov [ctty_rmod_err], ax
    mov bx, [ctty_saved_handle]
    mov ah, 0x3e
    int 0x21
    jmp RMOD_STACK_BACK

    RMOD_OPEN_FAIL:
    mov [ctty_rmod_err], ax

    RMOD_STACK_BACK:
    mov ax, [ctty_saved_ss]
    mov bx, [ctty_saved_sp]
    cli
    mov ss, ax
    mov sp, bx
    sti

    mov ah, 0x50
    mov bx, [ctty_saved_psp]
    int 0x21

    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
  }

  return(ctty_rmod_err);
}


/* make handles 0, 1 and 2 duplicates of handle. handle stays open. */
static unsigned short ctty_dup2_stdio(unsigned short handle) {
  unsigned short errcode = 0;

  _asm {
    push ax
    push bx
    push cx

    mov bx, [handle]
    xor cx, cx
    mov ah, 0x46
    int 0x21
    jc FAIL

    mov bx, [handle]
    mov cx, 1
    mov ah, 0x46
    int 0x21
    jc FAIL

    mov bx, [handle]
    mov cx, 2
    mov ah, 0x46
    int 0x21
    jnc DONE

    FAIL:
    mov [errcode], ax

    DONE:
    pop cx
    pop bx
    pop ax
  }

  return(errcode);
}


static void ctty_close(unsigned short handle) {
  _asm {
    push ax
    push bx
    mov bx, [handle]
    mov ah, 0x3e
    int 0x21
    pop bx
    pop ax
  }
}


static enum cmd_result cmd_ctty(struct cmd_funcparam *p) {
  unsigned short handle = 0xffff;
  unsigned short errcode;

  if (cmd_ishlp(p)) {
    nls_outputnl(41,0); /* "Changes the terminal device used to control your system." */
    outputnl("");
    nls_outputnl(41,1); /* "CTTY device" */
    outputnl("");
    nls_outputnl(41,2); /* "device  The terminal device you want to use, such as COM1." */
    return(CMD_OK);
  }

  if (p->argc > 1) {
    nls_outputnl(0,4); /* "Too many parameters" */
    return(CMD_FAIL);
  }

  if (p->argc == 0) {
    nls_outputnl(41,3); /* "Invalid device" */
    return(CMD_FAIL);
  }

  /* validate first, and install the device in RMOD before touching this
   * process, so a failure leaves the current console in place */
  errcode = ctty_open_device(p->argv[0], &handle);
  if (errcode == 0) errcode = ctty_apply_rmod(p->argv[0], p->rmod->rmodseg);
  if (errcode == 0) errcode = redir_point_saved(handle);
  if (errcode == 0) errcode = ctty_dup2_stdio(handle);
  if (handle != 0xffff) ctty_close(handle);

  if (errcode != 0) {
    nls_outputnl(41,3); /* "Invalid device" */
    return(CMD_FAIL);
  }

  return(CMD_OK);
}

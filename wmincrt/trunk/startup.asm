; WMINCRT - minimal runtime for (Open) Watcom C to generate DOS .COM files
;           and small memory model .EXE MZ executables
;
; The default feature set / behaviour of the runtime is:
;   - sets up a stack of 400H bytes
;   - releases additional memory beyond stack to DOS
;   - processes command line arguments into argc, argv
;   - stack based dynamic memory allocation
;   - panics on initialization, if not enough memory for stack size
;   - panics if running out of stack during execution
;
; WASM definitions:
;   EXE                   assemble for use with small memory model .EXE
;   DEBUG                 assemble for debugging (nullptr checks etc.)
;   STACKSIZE=<value>     define stack size other than 400h
;   NOARGV                disables argc, argv setup
;   NOSTACKALLOC          disables the stack allocator
;   NOSTACKCHECK          do not assemble __STK function
;   STACKSTAT             remember the minimum SP, maintained by __STK__
;                         and exported via _crt_stack_low.
;
; To build an executable without build- and linker scripts run for .COM:
;   wasm startup.asm
;   wcc -0 -bt=dos -ms -os -zl your_c_file.c
;   wlink system dos com file startup,your_c_file
;
; To build an .EXE program:
;   wasm -DEXE startup.asm
;   wcc -0 -bt=dos -ms -os -zl your_c_file.c
;   wlink system dos file startup,your_c_file
;
; To build a debug version of your program:
;   wasm -d1 -DDEBUG startup.asm
;   wcc -bt=dos -ms -d2 -zl your_c_file.c
;   wlink system dos com debug all option map file startup,your_c_file
;
; To debug .EXE files:
;   wasm -d1 -DDEBUG -DEXE startup.asm
;   wcc -bt=dos -ms -d2 -zl your_c_file.c
;   wlink system dos debug all option map file startup,your_c_file
;
; To change the stack size, add -DSTACKSIZE=<value> to the wasm call
;
; To compile without stack checking:
;   a) compile this startup file with wasm -DNOSTACKCHECK startup.asm
;   b) call the C compiler with -s flag or optimize via -ox

;
; TODO:
;   - display stack statistics on program termination if STACKSTAT is enabled
;   - proper Makefiles
;   - many more (optional) things while keeping it small :-)

.8086

IFNDEF STACKSIZE
STACKSIZE = 400h
ENDIF

STK_ALLOC_MARGIN equ 100h  ; stay in distance to SP when allocating


INCLUDE "segments.inc"        ; establish segment order


ASSUME DS:DGROUP,ES:DGROUP

      extrn   "C",main : near


;-----------------------------------------------------------------------------
; DGROUP: DATA, BSS , STACK etc.

_DATA segment

  
      public stk_bottom
stk_bottom dw offset DGROUP:_STACK

      public _crt_cmdline
  IFDEF EXE
    _crt_cmdline dw offset DGROUP:cmdline
  ELSE ; EXE
    _crt_cmdline dw 80h  ; point to PSP if .COM file
  ENDIF ; EXE

_DATA ends

_BSS segment

      public _crt_psp_seg

_crt_psp_seg dw ?             ; segment of PSP

  IFDEF EXE
    cmdline db 80h dup (?)    ; used as cmdline buffer for .EXE, otherwise PSP
  ENDIF ; EXE

_BSS  ends

; stack definition, available memory is checked at runtime
; defined as segment so that linker may detect .COM size overflow
_STACK segment

      public _stack_start_
      public _stack_end_

_stack_start_:
      db STACKSIZE dup(?)

_stack_end_:
_STACK ends


;-----------------------------------------------------------------------------
; COMMON CODE FOR ALL CONFIGURATIONS

_TEXT segment

      public _cstart_
      public _small_code_
      public crt_exit_

  IFNDEF EXE
      org   100h
  ENDIF

_small_code_ label near

_cstart_ proc
      ; DOS puts the COM program in the largest memory block it can find
      ; and sets SP to the end of this block. On top of that, it reserves
      ; the entire memory (not only the process' block) to the program, which
      ; makes it impossible to allocate memory or run child processes.
      ; for this reasons it is beneficial to resize the memory block we occupy
      ; into a more reasonable value

  IFDEF EXE
      mov ax,seg DGROUP
      mov ds,ax
      mov es,ax
      ; adjust SS, SP to our DGROUP
      mov ss,ax
      mov sp,offset DGROUP:_stack_end_
      mov [_crt_cmdline], offset DGROUP:cmdline
  ELSE EXE
    CONST segment
      memerr_msg db 'MEMERR$'
    CONST ends
    @verify_stack_size:
      cmp sp,offset DGROUP:_stack_end_
      ja @resize_mem
      mov dx,offset DGROUP:memerr_msg
      jmp panic

      ; step 2: resize our memory block to sp bytes (ie. sp/16 paragraphs)
    @resize_mem:
      mov sp,offset DGROUP:_stack_end_
      mov ah,4ah
      mov bx,sp
      add bx,0fh
      mov cl,4
      shr bx,cl
      int 21h
  ENDIF EXE

  IFDEF DEBUG
    IFDEF EXE
      ; initialize NULLPTR guard area
      mov ax,NULLGUARD_VAL
      mov cx,NULLGUARD_COUNT
      mov di,offset DGROUP:__nullarea
      cld
      rep stosw
    ENDIF EXE
  ENDIF DEBUG

  IFDEF STACKSTAT
      mov [_crt_stack_low],sp
  ENDIF STACKSTAT

      ; clear _BSS to be ANSI C conformant
      mov di,offset DGROUP:_BSS
      mov cx,offset DGROUP:_STACK+1       ; +1 for rounding up against 2
      sub cx,di
      shr cx,1
      xor ax,ax
      cld
      rep stosw

      ; get our PSP
      mov ah,51h
      int 21h
      mov [_crt_psp_seg],bx

  IFDEF EXE
      ; copy command line
      mov ds,bx
      mov si,80h
      mov di,offset DGROUP:cmdline
      mov cx,40h
      cld
      rep movsw
      push es
      pop ds
  ENDIF EXE

  IFNDEF NOARGV

      INCLUDE "argv.inc"                ; optional AX=argc, DX=argv handling

      mov ax,[__argc]
      mov dx,[__argv]
  ENDIF

      call main

      ; fallthrough to crt_exit_
_cstart_ endp


crt_exit_ proc
  IFDEF DEBUG

  CONST segment
      public nullguard_msg
      nullguard_msg db 'NULLPTR guard detected write to null area!',13,10,'$'
  CONST ends

      call crt_nullarea_check_
      cmp byte ptr [crt_nullarea_writes],0
      je @exit_done
      mov dx,offset DGROUP:nullguard_msg
      jmp panic
@exit_done:
  ENDIF DEBUG
      mov ah,4ch
      int 21h
crt_exit_ endp


panic proc
      ; output error message in DX, then terminate with FF
      mov ah,9
      int 21h
      mov ax,4c7fh      ; terminate with error code 7f
      int 21h
panic endp

  IFNDEF NOSTACKALLOC
    INCLUDE "alloc.inc"
  ENDIF

_TEXT ends


;-----------------------------------------------------------------------------
; DEBUGGING AND STACK CHECKING ROUTINES

IFDEF EXE
  NULLGUARD_VAL  equ 0CCCCh   ; INT3 opcode takes us to debugger if we
                              ;  erronerously call into the null area
  NULLGUARD_COUNT  equ 80h    ; check first 256 bytes to catch ill-targeted
                              ;  writes that should go to PSP
ELSE
  NULLGUARD_VAL  equ 20CDh    ; for .COM we check for INT20 in first two
                              ; bytes of PSP
  NULLGUARD_COUNT  equ 1
ENDIF

IFDEF STACKSTAT
  IFDEF NOSTACKCHECK
    ERROR "stack statistics requires NOSTACKCHECK to be undefined!"
  ENDIF
ENDIF

IFDEF DEBUG
  IFDEF EXE
    BEGTEXT segment word public 'CODE'
      int 3                   ; catch jumps / calls via NULL ptr
    BEGTEXT ends
  ENDIF
ENDIF ENDIF

_TEXT segment

  IFNDEF NOSTACKCHECK

      public __STK

  CONST segment
      stkerr_msg db 'STKERR',13,10,'$'
  CONST ends

__STK proc
      ; ensures that we have enough stack space left. the needed bytes are
      ; given in AX. panics if stack low.
      sub ax,sp         ; subtract needed bytes from SP
      neg ax            ; SP-AX = -(AX-SP)
      add ax,2          ; +2 is to compensate for __STK ret addr
      cmp ax,[stk_bottom]
      jae @stk_stat     ; enough stack => return, else panic
  IFDEF DEBUG
      int 3             ; give debugger chance to trap
  ENDIF DEBUG
      mov dx,offset DGROUP:stkerr_msg
      add sp,200h       ; make sure we have enough stack to call DOS
      jmp panic
@stk_stat:
  IFDEF STACKSTAT       ; update lowest stack pointer if statistics enabled
    _BSS segment

      public _crt_stack_low

      _crt_stack_low:  dw 1 dup(?)
    _BSS ends

      cmp [_crt_stack_low],ax
      jbe @stk_r
      mov [_crt_stack_low],ax
  ENDIF STACKSTAT
@stk_r:
      ret
__STK endp

  ENDIF NOSTACKCHECK

  IFDEF DEBUG

      public crt_nullarea_check_

  _DATA segment
      crt_nullarea_writes db 0
  _DATA ends

crt_nullarea_check_ proc
  ; checks to nullarea for corruption, triggers INT3 on failed check
  ; returns 0 on corrupted nullarea, otherwise 1
  ; returns ZF cleared on corruption, otherwise set

    IFDEF EXE
      ; check for writing NULL ptr
      push ax
      push cx
      push di
      mov ax,NULLGUARD_VAL
      mov cx,NULLGUARD_COUNT
      mov di,offset DGROUP:__nullarea
      push es
      push ds
      pop es
      cld
      repe scasw
      pop es
      pop di
      pop cx
      pop ax
      je @chk_done          ; no magic words changed, assume no NULL ptr write
    ELSE
      cmp word ptr ds:[0],NULLGUARD_VAL
      je @chk_done
    ENDIF
    mov crt_nullarea_writes,1
    int 3
@chk_done:
    ret
crt_nullarea_check_ endp

  ENDIF DEBUG

_TEXT ends

IFDEF DEBUG

  IFDEF EXE

  _NULL segment
      public  __nullarea     ; Watcom Debugger needs this!!!
  __nullarea label word

      dw NULLGUARD_COUNT dup (NULLGUARD_VAL)

  _NULL   ends

  _AFTERNULL segment
      dw 0                  ; nullchar for string at address 0
  _AFTERNULL ends

  ENDIF EXE

ENDIF DEBUG

      end _cstart_

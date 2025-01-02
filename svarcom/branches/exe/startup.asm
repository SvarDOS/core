; WMINICRT - minimal runtime for (Open) Watcom C to generate DOS .COM files
;
; The default behaviour of the runtime is:
;   - sets up a stack of 400H bytes
;   - releases additional memory beyond stack to DOS
;   - panics on initialization, if not enough memory for stack size
;   - panics if running out of stack during execution
;
; WASM definitions:
;   STACKSIZE=<value>     define stack size other than 400h
;   NOSTACKCHECK          do not assemble __STK function
;   STACKSTAT             remember the minimum SP, maintained by __STK__
;                         and exported via _stack_low_.
;
; To build an executable without build- and linker scripts run:
;   wasm startup.asm
;   wcc -0 -bt=dos -ms -os -zl your_c_file.c
;   wlink system dos com file startup,your_c_file
; To change the stack size, add -DSTACKSIZE=<value> to the wasm call
;
; To compile without stack checking:
;   a) compile this startup file with wasm -DNOSTACKCHECK startup.asm
;   b) call the C compiler with -s flag
;
; To build a debug version of your program:
;   wasm -d1 startup.asm
;   wcc -bt=dos -ms -d2 -zl your_c_file.c
;   wlink system dos com debug all option map file startup,your_c_file
;
; TODO:
;   - display stack statistics on program termination if STACKSTAT is enabled
;   - proper Makefiles
;   - many more (optional) things while keeping it small :-)

.8086


IFDEF DEBUG
  IFNDEF EXE
    EXE EQU 1
  ENDIF
ENDIF

NULLGUARD_VAL    equ 0101h
NULLGUARD_COUNT  equ 80h      ; check first 256 bytes for writes

IFNDEF STACKSIZE
STACKSIZE = 400h
ENDIF

IFDEF EXE
      DGROUP group _NULL,_AFTERNULL,CONST,CONST2,STRINGS,_DATA,_BSS,_STACK
ELSE
      DGROUP group _TEXT,_DATA,CONST,CONST2,_BSS,_STACK
ENDIF

ASSUME DS:DGROUP,ES:DGROUP

      extrn   "C",main : near

IFDEF EXE
BEGTEXT segment word public 'CODE'
      dw 10 dup(?)
BEGTEXT ends
ENDIF

_TEXT segment word public 'CODE'

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
      mov [_crt_temp_dta],offset DGROUP:dta
      mov [_crt_temp_cmdline], offset DGROUP:cmdline
  ELSE
    @verify_stack_size:
      cmp sp,offset DGROUP:_stack_end_
      ja @resize_mem
      mov dx,offset DGROUP:memerr_msg
      jmp _panic_

      ; step 2: resize our memory block to sp bytes (ie. sp/16 paragraphs)
    @resize_mem:
      mov sp,offset DGROUP:_stack_end_
      mov ah,4ah
      mov bx,sp
      add bx,0fh
      shr bx,1
      shr bx,1
      shr bx,1
      shr bx,1
      int 21h
  ENDIF

  IFDEF DEBUG
      ; initialize NULLPTR guard area
      mov ax,NULLGUARD_VAL
      mov cx,NULLGUARD_COUNT
      mov di,offset DGROUP:__nullarea
      cld
      rep stosw
  ENDIF

  IFDEF STACKSTAT
      mov [_stack_low_],sp
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
      ; set disk transfer address
      mov ah,1ah
      mov dx,[_crt_temp_dta]
      int 21h

      ; copy command line
      mov ds,bx
      mov si,80h
      mov di,offset DGROUP:cmdline
      mov cx,40h
      cld
      rep movsw
      push es
      pop ds
  ENDIF

      call main

      jmp crt_exit_
_cstart_ endp

crt_exit_ proc
  IFDEF DEBUG
      ; check for writing NULL ptr
      mov bx,ax
      mov ax,NULLGUARD_VAL
      mov cx,NULLGUARD_COUNT
      mov di,offset DGROUP:__nullarea
      push ds
      pop es
      cld
      repe scasw
      mov ax,bx
      jz @done          ; no magic words changed, assume no NULL ptr write
      mov dx,offset DGROUP:nullguard_msg
      jmp _panic_
  ENDIF
@done:
      mov ah,4ch
      int 21h
crt_exit_ endp

_panic_ proc
      ; output error message in DX, then terminate with FF
      mov ah,9
      int 21h
      mov ax,4cffh      ; terminate if not enough stack space
      int 21h
_panic_ endp

      IFNDEF NOSTACKCHECK

public __STK
__STK proc
      ; ensures that we have enough stack space left. the needed bytes are
      ; given in AX. panics if stack low.
      sub ax,sp         ; subtract needed bytes from SP
      neg ax            ; SP-AX = -(AX-SP)
      cmp ax,offset DGROUP:_STACK - 2 ; -2 is to compensate for __STK ret addr
      jae @l1           ; enough stack => return, else panic
      int 3             ; trap into debugger
      mov dx,offset DGROUP:stkerr_msg
      add sp,200h       ; make sure we have enough stack to call DOS
      jmp _panic_
@l1:
      IFDEF STACKSTAT   ; update lowest stack pointer if statistics enabled
        cmp [_stack_low_],ax
        jbe @r
        mov [_stack_low_],ax
      ENDIF STACKSTAT
@r:
      ret
__STK endp

      ENDIF

FAR_DATA segment byte public 'FAR_DATA'
FAR_DATA ends


_NULL   segment para public 'BEGDATA'

__nullarea label word
        public  __nullarea              ; Watcom Debugger needs this!!!
  IFDEF DEBUG
      dw      NULLGUARD_VAL dup(NULLGUARD_COUNT)
  ENDIF
_NULL   ends

_AFTERNULL segment word public 'BEGDATA'
      dw      0                       ; nullchar for string at address 0
_AFTERNULL ends


CONST segment word public 'DATA'

memerr_msg db 'MEMERR$'
stkerr_msg db 'STKERR$'

  IFDEF DEBUG
      nullguard_msg db 'NULLPTR guard detected write to null area!$'
  ENDIF
CONST ends

CONST2 segment word public 'DATA'
CONST2 ends

STRINGS segment word public 'DATA'
STRINGS ends

_DATA segment word public 'DATA'

      public _crt_temp_dta
      public _crt_temp_cmdline

  IFDEF EXE
    _crt_temp_dta     dw offset DGROUP:dta
    _crt_temp_cmdline dw offset DGROUP:cmdline
  ELSE
    _crt_temp_dta     dw 80h  ; point to PSP if .COM file
    _crt_temp_cmdline dw 80h  ; point to PSP if .COM file
  ENDIF

_DATA ends

_BSS segment word public 'BSS'

      public _crt_psp_seg

_crt_psp_seg dw ?             ; segment of PSP

  IFDEF STACKSTAT
    public _stack_low_
    _stack_low_:  dw 1 dup(?)
  ENDIF STACKSTAT

  IFDEF EXE
    dta label byte
    cmdline db 80h dup (?) ; used as DTA etc. when compiled as .EXE
  ENDIF
_BSS  ends

; stack definition, available memory is checked at runtime
; defined as segment so that linker may detect .COM size overflow
IFDEF EXE
_STACK segment para public 'STACK'
ELSE
_STACK segment para public 'TINY_STACK'
ENDIF

public _stack_end_
      db STACKSIZE dup(?)
_stack_end_:
_STACK ends

_TEXT ends

      end _cstart_

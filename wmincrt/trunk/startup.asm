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

IFNDEF STACKSIZE
STACKSIZE = 400h
ENDIF

      DGROUP group _TEXT,_DATA,CONST,CONST2,_BSS,_STACK

      extrn   "C",main : near

      public _cstart_, _small_code_

_TEXT segment word public 'CODE'
      org   100h

_small_code_ label near

_cstart_ proc
      ; DOS puts the COM program in the largest memory block it can find
      ; and sets SP to the end of this block. On top of that, it reserves
      ; the entire memory (not only the process' block) to the program, which
      ; makes it impossible to allocate memory or run child processes.
      ; for this reasons it is beneficial to resize the memory block we occupy
      ; into a more reasonable value

    @verify_stack_size:
      cmp sp,offset DGROUP:_stack_end_
      ja @resize_mem
      mov dx,offset @memerr
      jmp _panic_
      @memerr db 'MEMERR$'

      ; step 2: resize our memory block to sp bytes (ie. sp/16 paragraphs)
    @resize_mem:
      mov sp,offset DGROUP:_stack_end_
      IFDEF STACKSTAT
        mov [_stack_low_],sp
      ENDIF STACKSTAT
      mov ah,4ah
      mov bx,sp
      add bx,0fh
      shr bx,1
      shr bx,1
      shr bx,1
      shr bx,1
      int 21h

      ; clear _BSS to be ANSI C conformant
      mov di,offset DGROUP:_BSS
      mov cx,offset DGROUP:_STACK
      sub cx,di
      shr cx,1
      xor ax,ax
      cld
      rep stosw

      call main
      mov ah,4ch
      int 21h
_cstart_ endp

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
      mov dx,offset @stkerr
      add sp,200h       ; make sure we have enough stack to call DOS
      jmp _panic_
      @stkerr db 'STKERR$'
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

_DATA segment word public 'DATA'
_DATA ends

CONST segment word public 'DATA'
CONST ends

CONST2 segment word public 'DATA'
CONST2 ends

_BSS  segment word public 'BSS'
      IFDEF STACKSTAT
        public _stack_low_
        _stack_low_:  dw 1 dup(?)
      ENDIF STACKSTAT
_BSS  ends

; stack definition, available memory is checked at runtime
; defined as segment so that linker may detect .COM size overflow
_STACK segment para public 'TINY_STACK'
public _stack_end_
      db STACKSIZE dup(?)
_stack_end_:
_STACK ends

_TEXT ends

      end _cstart_

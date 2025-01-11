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
;   PRGNAME               fill argv[0] canonicalized program file name
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

include "common.inc"

ASSUME DS:DGROUP,ES:DGROUP

      extrn   "C",main : near


;-----------------------------------------------------------------------------
; DGROUP: DATA, BSS , STACK etc.

startdata

  public _crt_stk_bottom
_crt_stk_bottom dw offset DGROUP:_STACK

      public _crt_cmdline
  IFDEF EXE
    _crt_cmdline dw offset DGROUP:cmdline
  ELSE ; EXE
    _crt_cmdline dw 80h  ; point to PSP if .COM file
  ENDIF ; EXE

IFDEF DEBUG
  IFDEF EXE
      dw ___begtext       ; make sure BEG_TEXT segment is not eliminated
  ENDIF EXE
ENDIF DEBUG

enddata

startbss

      public _crt_psp_seg
_crt_psp_seg dw ?             ; segment of PSP

  IFDEF EXE
    cmdline db 80h dup (?)    ; used as cmdline buffer for .EXE, otherwise PSP
  ENDIF ; EXE

endbss

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

startcode

      public _cstart_
      public _small_code_
      public crt_exit_

  IFNDEF EXE
      org   100h
  ENDIF

_small_code_ label near

pubproc _cstart_
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
      jmp crt_panic_

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

      ; We process the command line given to us by the PSP into the
      ; __argc and __argv variables.

      ; First byte of _crt_cmdline contains the length of the command line.
      ; The cmdline provided to us by the PSP may or may not be terminated
      ; by a carriage return (0ch).
      ; It may be 7fh bytes at max. If it is 7fh bytes, there is no space
      ; for a terminating zero, therefore we error out if it is equal or more
      ; than 7fh bytes long.

      ; Whitespaces isolate command line arguments. We filter them out.
      ; All characters <= 20h are considered to be whitespace.

  _DATA segment
      public __argc
      public _crt_argc

    _crt_argc label word
    __argc dw 1          ; number or arguments, first is program name

    IFDEF PRGNAME
      prg_name db '        ',0
    ELSE
      prg_name db '',0
    ENDIF
  _DATA ends

  _BSS segment
      public __argv
      public _crt_argv

    _crt_argv label word
    __argv dw ?          ; pointer to array of cmdline argument pointers
  _BSS ends

      xor bx,bx                     ; use bx as zero store for rest of .INC

      ; We place the argv pointer array on the bottom of the stack.
      ; We loose a few bytes of stack space, but that should be ok.
      mov di,offset DGROUP:_STACK
      ; ARGV[0] is typically the name of the executable, currently set to NULL
      ; TODO: implement properly
      mov [__argv],di
      mov [di],offset DGROUP:prg_name
      inc di
      inc di
      ; load cmdline length and verify we can deal with it
      mov si,[_crt_cmdline]
      lodsb                         ; load cmdline length should be 7fh at max                
      mov cl,al
      xor dh,dh
      cmp cl,07fh                   ; make sure there is space for trailing zero
      jae @arg_done                 ; or else return empty argv
@arg_skip_ws:
      ; Skip over whitespace characters while replacing them by zeroes.
      ; This impliciyly zero-terminates the preceeding argument.
      mov al,[si]                   ; get next chararcter
      cmp al,' '                    ; is it a whitespace?
      ja @arg_start                 ; if not, argument starts
      cmp al,13                     ; CR terminates the command line
      je @arg_done
      mov [si],bl                   ; replace whitespace with zero
      dec cx                        ; one character less
      jcxz @arg_done                ; no more characters?
      inc si                        ; next character
      jmp @arg_skip_ws              ; contine whitespace skipping
@arg_start:
      inc [__argc]
      mov [di],si                   ; store next ARGV pointer
      inc di
      inc di
@arg_loop:
      mov al,[si]                   ; get next character
      cmp al,' '                    ; is it a whitespace?
      jbe @arg_skip_ws              ; if yes, skip over it
      inc si
      loop @arg_loop
@arg_done:
      mov [si],bl                   ; zero-terminate last argument
      mov word ptr [di],bx          ; zero-terminate argv ptr array
      inc di                        ; adjust bottom of stack
      inc di
      mov [_crt_stk_bottom],di

    IFNDEF NOPRGNAME
      ; get absolute PATH to executable and put it into ARGV[0]
      ; the string itself snacks from the bottom stack
      push ds
      push ds
      pop es
      mov ax,[_crt_psp_seg]
      mov ds,ax                     ; PSP seg into ds
      mov ax,[bx+2ch]
      mov ds,ax                     ; ENV seg into ds
      xor ax,ax
      xor si,si
@env_prgname:
      ; programs executable absolute path follows environment terminated
      ; by two zeroes
      mov ax,[si]
      inc si
      test ax,ax
      jnz @env_prgname
      lodsb                         ; skip second zero
      lodsw                         ; skip string count
      ; si now points to executable file name
      mov ah,60h                    ; call TRUENAME on file name
      int 21h                       ; to canonicalize and copy to bottom
      pop ds                        ; restore our ds
      jc @env_prgname_done          ; leave ARGV[0]="" on error
      mov si,di                     ; let si point to canonicalized name
@env_prgname_to_end:
      lodsb                         ; and determine end of name
      test al,al                    ; by testing for zero
      jnz @env_prgname_to_end    
      inc si                        ; skip terminating zero
      ; update bottom of stack and argv[0]
      mov ax,[_crt_stk_bottom]      ; get start of program path
      mov di,[__argv]               ; get argv array
      mov [di],ax                   ; put program path address into argv[0]
      mov [_crt_stk_bottom],si      ; update bottom of stack
@env_prgname_done:
    ENDIF NOPRGNAME

      mov ax,[__argc]
      mov dx,[__argv]
  ENDIF NOARGV

      call main

      ; fallthrough to crt_exit_
endproc _cstart_


pubproc crt_exit_
  IFDEF DEBUG

  CONST segment
      public nullguard_msg
      nullguard_msg db 'NULLPTR guard detected write to null area!',13,10,'$'
  CONST ends

      call crt_nullarea_check_
      cmp byte ptr [crt_nullarea_writes],0
      je @exit_done
      mov dx,offset DGROUP:nullguard_msg
      jmp crt_panic_
@exit_done:
  ENDIF DEBUG
      mov ah,4ch
      int 21h
endproc crt_exit_

      public crt_panic_
pubproc crt_panic_
      ; output error message in DX, then terminate with FF
      mov ah,9
      int 21h
      mov ax,4c7fh      ; terminate with error code 7f
      int 21h
endproc crt_panic_

endcode


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
    public ___begtext
    ___begtext label byte
      int 3                   ; catch jumps / calls via NULL ptr
    BEGTEXT ends
  ENDIF
ENDIF ENDIF

_TEXT segment

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

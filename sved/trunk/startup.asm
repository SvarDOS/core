.8086

STACK_SIZE = 2048

      dgroup group _TEXT,_DATA,CONST,CONST2,_STACK,_BSS

      extrn   "C",main : near

;      public _cstart_, _small_code_, __STK
      public _cstart_, _small_code_

_TEXT segment word public 'CODE'
      org   100h

_small_code_ label near

_cstart_:

      ; DOS puts the COM program in the largest memory block it can find
      ; and sets SP to the end of this block. On top of that, it reserves
      ; the entire memory (not only the process' block) to the program, which
      ; makes it impossible to allocate memory or run child processes.
      ; for this reasons it is beneficial to resize the memory block we occupy
      ; into a more reasonable value

      ; step 1: if SP is higher than my top_of_stack, then set SP explicitely
      cmp sp, top_of_stack
      jle resizemem
      mov sp, top_of_stack

      ; step 2: resize our memory block to sp bytes (ie. sp/16 paragraphs)
    resizemem:
      mov ah, 4ah
      mov bx, sp
      shr bx, 1
      shr bx, 1
      shr bx, 1
      shr bx, 1
      inc bx
      int 21h

      call  main
      mov   ah, 4ch
      int   21h

; Stack overflow checking routine is absent. Remember to compile your
; programs with the -s option to avoid referencing __STK
;__STK:
;      ret

_DATA   segment word public 'DATA'
_DATA   ends

CONST   segment word public 'DATA'
CONST   ends

CONST2  segment word public 'DATA'
CONST2  ends

_BSS    segment word public 'BSS'
_BSS    ends

_STACK  segment para public 'BSS'
        db      (STACK_SIZE) dup(0) ; set this explicitely to zero, otherwise
                                    ; static variables are not properly
                                    ; initialized. this makes the COM file
                                    ; much bigger, but it is irrelevant if it
                                    ; is UPXed afterwards anyway. If you care,
                                    ; then you may zero out this area in
                                    ; code instead (before calling main)
        top_of_stack:
_STACK  ends

_TEXT ends

      end _cstart_

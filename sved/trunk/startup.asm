; minimalist Watcom C startup routine - TINY memory model ONLY - (.COM files)
; kindly contributed by Bernd Boeckmann

.8086

STACK_SIZE = 2048

      DGROUP group _TEXT,_DATA,CONST,CONST2,_BSS,_EOF

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

      ; step 1: if SP > COM size + stack size, then set SP explicitely
      ; POTENTIAL error: offset DGROUP:_EOF + STACK_SIZE may overflow!
      ; Has to be detected at linking stage, but WLINK does not catch it.
    adjustsp:
      cmp sp, offset DGROUP:_EOF + STACK_SIZE
      jbe resizemem   ; JBE instead of JLE (unsigned comparison!)
      mov sp, offset DGROUP:_EOF + STACK_SIZE

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

      ; clear _BSS to be ANSI C conformant
      mov di, offset DGROUP:_BSS
      mov cx, offset DGROUP:_EOF
      sub cx, di
      xor al, al
      cld
      rep stosb

      call  main
      mov   ah, 4ch
      int   21h

; Stack overflow checking routine is absent. Remember to compile your
; programs with the -s option to avoid referencing __STK
;__STK:
;      ret

_DATA segment word public 'DATA'
_DATA ends

CONST segment word public 'DATA'
CONST ends

CONST2 segment word public 'DATA'
CONST2 ends

_BSS  segment word public 'BSS'
_BSS  ends

; _EOF should be the last segment in .COM (see linker memory map)
; if not, someone introduced other segments, or startup.obj is not the first
; linker object file
_EOF  segment word public
_EOF  ends

_TEXT ends

      end _cstart_


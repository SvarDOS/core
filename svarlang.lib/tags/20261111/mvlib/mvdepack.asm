;
; MVCOMP depacking routine in 8086 assembly
; Copyright (C) 2024-2026 Mateusz Viste, MIT license
;
; http://mateusz.fr/mvcomp/
;
; this is NOT a standalone program, but only a routine to be used within your
; own code.
;
; compatible with nasm, tasm, wasm, fasm, a72, etc.
; assembles to 54 bytes of 8086 machine code:
; nasm -f bin mvdepack.asm -o mvdepack.bin
;
; parameters:
; ES:SI  the source buffer (start of MVCOMP compressed stream)
; ES:DI  the dst buffer (may overlap with the source buffer, see ../in-place.txt)
; BX     the length, in bytes, of the compressed stream (even)
;
; On exit, DI points one byte past the last uncompressed byte (subtract its old
; value to get the amount of data uncompressed).
; Registers AX, BX, CX and SI are destroyed. DX, BP and ES are preserved.
; DS is restored. DF is left clear.
;
; A literal whose raw words are not all inside the remaining input is not
; written, and no byte past the given length is read.
;

    push ds
    push es
    pop ds
    cld                ; make sure stosw and friends move forward
    shr bx, 1          ; convert byte length to number of words
    jz KONIEC

AGAIN:

    lodsw
    test ax, 0f000h
    jz LITERAL

    ; token format: LLLL OOOO OOOO OOOO
    push si
    mov ch, ah         ; all this drama is about setting CX to the high
    mov cl, 4          ; nibble of AX using code as compact as possible.
    shr ch, cl
    mov cl, ch
    xor ch, ch
    inc cx
    and ax, 0fffh      ; clear the backref length bits, keep only the offset
    inc ax             ; mask first: 0x0FFF + 1 must stay 4096
    mov si, di
    sub si, ax
    rep movsb          ; copy cx bytes from ds:si to es:di + inc si + inc di
    pop si
    jmp short NEXT

    ; token format: 0000 UUUU  BBBB BBBB
LITERAL:
    mov cl, ah         ; AH is UUUU; the top nibble tested zero
    xor ch, ch
    sub bx, cx         ; BX still counts the word just loaded
    jbe KONIEC         ; need a spare word for this token (BX > CX)
    stosb
    rep movsw          ; CX = 0 (UUUU = 0) copies nothing

NEXT:

    dec bx
    jnz AGAIN

KONIEC:
    pop ds

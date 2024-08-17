; Installs an int24h handler that always fail, so DOS does not output ugly
; "abort, retry, fail" messages.
;
; This file is part of the SvarDOS INSTALL program. MIT license.
; Copyright (C) 2022 Mateusz Viste
;
; Small memory model only.
;
; This code is based on the template provided in the Watcom user guide in
; section 7.4.5 ("Interfacing to Assembly Language Functions"), p. 138.
;

DGROUP    group _DATA
_TEXT     segment byte public ’CODE’
          assume CS:_TEXT
          assume DS:DGROUP
          public int24hdl_

int24hdl_ proc near
; === function starts =========================================================

push ax        ; save the registers that I change
push dx
push ds

mov ax, cs
mov ds, ax
mov ax, 0x2524 ; Set INT vector 0x24 to DS:DX
mov dx, offset HANDLER_FAIL
int 0x21

pop ds         ; restore all registers back to their initial state
pop dx
pop ax
ret            ; return from function

HANDLER_FAIL:
mov al, 3      ; tell DOS the action failed (DOS 3.0+)
iret           ; return from interrupt handler

; === function ends ===========================================================
int24hdl_ endp
_TEXT     ends

end

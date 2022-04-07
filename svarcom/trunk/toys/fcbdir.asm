;
; lists files using first or second FCB provided by COMMAND.COM in PSP
; Copyright (C) 2022 Mateusz Viste
;
; to be assembled with NASM.
;
; lists file entries matching first argument of the program using the default
; unopened FCB entry in PSP as preset by COMMAND.COM.
;
; usage: fcbdir file-pattern
;
; example: fcbdir c:*.txt
;

CPU 8086
org 0x100

; print whatever drive/filename is set at 0x5C (1st unopened FCB in the PSP)
mov dx, PARAM
mov ah, 0x09
int 0x21

mov bx, 0x5C
call PRINTDTA

; FindFirst_FCB applied to PSP:5C (1st unopened FCB)
mov dx, 0x5C
mov ah, 0x11
int 0x21

cmp al, 0
jne GAMEOVER

NEXT:

; print filename in DTA
mov bx, 0x80
call PRINTDTA

; FindNext_FCB on PSP until nothing left
mov ah, 0x12
mov dx, 0x5C
int 0x21
cmp al, 0
je NEXT

GAMEOVER:

int 0x20

PARAM:
db "PARAMETER = $";


; ****** print the filename present in the DTA (DTA is at [BX]) ******
PRINTDTA:
; print drive in the DTA
mov ah, 0x02
mov dl, [bx]
add dl, '@'
int 0x21
mov dl, ':'
int 0x21
; print filename/extension (in FCB format)
mov ah, 0x40    ; write to file
mov cx, 11      ; 11 bytes (8+3)
mov dx, bx      ; filename field at the default DTA location
inc dx
mov bx, 1       ; output to stdout
int 0x21
; print a trailing CR/LF
mov ah, 0x02    ; display character in DL
mov dl, 0x0D    ; CR
int 0x21
mov dl, 0x0A    ; LF
int 0x21
ret

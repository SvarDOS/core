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

; FindFirst_FCB applied to PSP:5C (1st unopened FCB)
mov dx, 0x5C
mov ah, 0x11
int 0x21

cmp al, 0
jne GAMEOVER

NEXT:

; print filename in DTA
call PRINTDTA

; FindNext_FCB on PSP until nothing left
mov ah, 0x12
mov dx, 0x5C
int 0x21
cmp al, 0
je NEXT

GAMEOVER:

int 0x20



; ****** print the filename present in the DTA (DTA is assumed at 0x80) ******
PRINTDTA:
; print drive in the DTA
mov ah, 0x02
mov dl, [0x80]
add dl, 'A'
int 0x21
mov dl, ':'
int 0x21
; print filename/extension (in FCB format)
mov ah, 0x40    ; write to file
mov bx, 1       ; output to stdout
mov cx, 11      ; 11 bytes (8+3)
mov dx, 0x81    ; filename field at the default DTA location
int 0x21
; print a trailing CR/LF
mov ah, 0x02    ; display character in DL
mov dl, 0x0D    ; CR
int 0x21
mov dl, 0x0A    ; LF
int 0x21
ret

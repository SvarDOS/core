;
; rmod - resident module of the SvarCOM command interpreter
;
; Copyright (C) 2021 Mateusz Viste
; MIT license
;
; this is installed in memory by the transient part of SvarCOM. it has only
; two jobs: providing a resident buffer for command history, environment, etc
; and respawning COMMAND.COM whenever necessary.

CPU 8086
org 0h           ; this is meant to be executed without a PSP

section .text    ; all goes into code segment

jmp short skipsig

SIG1 dw 0x1983
SIG2 dw 0x1985
SIG3 dw 0x2017
SIG4 dw 0x2019

; service routine: used by the transient part of svarcom, returns:
; AX = offset of input buffer history block
; BX = offset where environment's segment is stored (patched at install time)
inputroutine:
mov ax, BUF000
mov bx, ENVSEG
retf

skipsig:

; set up CS=DS=SS and point SP to my private stack buffer
mov ax, cs
mov ds, ax
mov es, ax
mov ss, ax
mov sp, STACKPTR

; prepare the exec param block
;mov [EXEC_PARAM_REC], word 0
mov ax, COMSPEC
mov [EXEC_PARAM_REC+2], ax
mov [EXEC_PARAM_REC+4], ds

; execute command.com
mov ax, 0x4B00         ; DOS 2+ - load & execute program
mov dx, COMSPEC        ; DS:DX  - ASCIZ program name TODO: use real COMSPEC...
mov bx, EXEC_PARAM_REC ; ES:BX  - parameter block pointer
int 0x21

; if all went well, jump back to start
jnc skipsig

; update error string so it contains the error number
add al, '0'
mov [ERRLOAD + 4], al

; display error message (with trailing COMSPEC)
mov ah, 0x09
mov dx, ERRLOAD
mov [COMSPCZ], byte '$' ; patch comspec terminator to be $
int 0x21
mov [COMSPCZ], byte 0   ; restore initial (NULL) compsec terminator

; wait for keypress
mov ah, 0x08
int 0x21

; back to program start
jmp skipsig

; ExecParamRec used by INT 21h, AX=4b00 (load and execute program), 14 bytes:
;  offset  size  content
;     +0     2   segment of environment for child (0 = current)
;     +2     4   address of command line to place at PSP:0080
;     +6     4   address of an FCB to be placed at PSP:005c
;    +0Ah    4   address of an FCB to be placed at PSP:006c
EXEC_PARAM_REC db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

ERRLOAD db "ERR x, FAILED TO LOAD COMMAND.COM FROM:", 13, 10

COMSPEC db "C:\SVN\SVARDOS\SVARCOM\COMMAND.COM"
COMSPCZ db 0

; input buffer used for the "previous command" history
BUF000 db 128, 0
BUF064 db "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
BUF128 db "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"

; FreeDOS int 21h functions that I use require at least 32 bytes of stack,
; here I allocate 64 bytes to be sure
STACKBUF db "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
STACKPTR db "xx"

; environment segment - this is updated by SvarCOM at init time
ENVSEG   dw 0

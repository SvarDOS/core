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
org 0x100

PSP_ENVSEG equ 0x2C

section .text    ; all goes into code segment

                 ; offset
SIG1 dw 0x1983   ;  +0
SIG2 dw 0x1985   ;  +2
SIG3 dw 0x2017   ;  +4
SIG4 dw 0x2019   ;  +6

FFU_UNUSED dw 0  ;  +8

; exit code of last application
LEXCODE  dw 0    ; +0Ah

; offset of the COMSPEC variable in the environment block, 0 means "use
; boot drive". this value is patched by the transient part of COMMAND.COM
COMSPECPTR dw 0  ; +0Ch

; fallback COMSPEC string used if no COMPSEC is present in the environment
; drive. drive is patched by the transient part of COMMAND.COM
COMSPECBOOT db "@:\COMMAND.COM", 0 ; +0Eh

; ExecParamRec used by INT 21h, AX=4b00 (load and execute program), 14 bytes:
;  offset  size  content
;     +0     2   segment of environment for child (0 = current)
;     +2     4   address of command line to place at PSP:0080
;     +6     4   address of an FCB to be placed at PSP:005c
;    +0Ah    4   address of an FCB to be placed at PSP:006c
EXEC_PARAM_REC db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   ; +1Dh

; Program to execute, preset by SvarCOM (128 bytes, ASCIIZ)  ; +2Bh
EXECPROG dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

skipsig:         ; +ABh

; set up CS=DS=SS and point SP to my private stack buffer
mov ax, cs
mov ds, ax
mov es, ax
mov ss, ax
mov sp, STACKPTR

; should I executed command.com or a pre-set application?
or [EXECPROG], byte 0
jz EXEC_COMMAND_COM

; TODO: perhaps I should call the DOS SetPSP function here? But if I do, the
;       int 21h, ah=50h call freezes...
;mov ah, 0x50           ; DOS 2+ -- Set PSP
;mov bx, cs
;int 0x21

; exec an application preset (by SvarCOM) in the ExecParamRec
mov ax, 0x4B00         ; DOS 2+ - load & execute program
mov dx, EXECPROG       ; DS:DX  - ASCIZ program name (preset at PSP[already)
mov bx, EXEC_PARAM_REC ; ES:BX  - parameter block pointer
int 0x21
mov [EXECPROG], byte 0 ; make sure to spawn command.com after app exits

EXEC_COMMAND_COM:

; collect the exit code of previous application
mov ah, 0x4D
int 0x21
xor ah, ah          ; clear out termination status, I only want the exit code
mov [LEXCODE], ax

; zero out the exec param block (14 bytes)
mov al, 0              ; byte to write
mov cx, 14             ; how many times
mov di, EXEC_PARAM_REC ; ES:DI = destination
cld                    ; stosb must move forward
rep stosb              ; repeat cx times

; preset the default COMSPEC pointer to ES:DX (ES is already set to DS)
mov dx, COMSPECBOOT

; do I have a valid COMSPEC?
or [COMSPECPTR], word 0
jz USEDEFAULTCOMSPEC
; set ES:DX to actual COMSPEC (in env segment)
mov es, [PSP_ENVSEG]
mov dx, [COMSPECPTR]
USEDEFAULTCOMSPEC:

; prepare the exec param block
mov ax, [PSP_ENVSEG]
mov [EXEC_PARAM_REC], ax
mov ax, CMDTAIL
mov [EXEC_PARAM_REC+2], ax
mov [EXEC_PARAM_REC+4], cs

; execute command.com
mov ax, 0x4B00         ; DOS 2+ - load & execute program
push es                ;
pop ds                 ;
;mov dx, COMSPEC       ; DS:DX  - ASCIZ program name (preset already)
push cs
pop es
mov bx, EXEC_PARAM_REC ; ES:BX  - parameter block pointer
int 0x21

; if all went well, jump back to start
jnc skipsig

; restore DS=CS
mov bx, cs
mov ds, bx

; update error string so it contains the error number
add al, '0'
mov [ERRLOAD + 4], al

; display error message
mov ah, 0x09
mov dx, ERRLOAD
int 0x21

; wait for keypress
mov ah, 0x08
int 0x21

; back to program start
jmp skipsig

; command.com tail arguments, in PSP format: length byte followed by args and
; terminated with \r)
CMDTAIL db 0x00, 0x0D

ERRLOAD db "ERR x, FAILED TO LOAD COMMAND.COM", 13, 10, '$'

; DOS int 21h functions that I use require at least 32 bytes of stack, here I
; allocate 64 bytes to be sure
STACKBUF db "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
STACKPTR db "xx"

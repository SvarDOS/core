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
SIG4 dw 0x2019   ;  +6  this acts also as a guardval to detect stack overflows

; DOS int 21h functions that I use require at least 40 bytes of stack under
; DOS-C (FreeDOS) kernel, so here I reserve 64 bytes juste to be sure
STACKBUF db "XXX  SVARCOM RMOD BY MATEUSZ VISTE  XXXXXXXXXXXXXXXXXXXXXXXXXXXX"
STACKPTR dw 0

; exit code of last application
LEXCODE  dw 0    ; +4Ah

; offset of the COMSPEC variable in the environment block, 0 means "use
; boot drive". this value is patched by the transient part of COMMAND.COM
COMSPECPTR dw 0  ; +4Ch

; fallback COMSPEC string used if no COMPSEC is present in the environment
; drive. drive is patched by the transient part of COMMAND.COM
COMSPECBOOT db "@:\COMMAND.COM", 0 ; +4Eh

; ExecParamRec used by INT 21h, AX=4b00 (load and execute program), 14 bytes:
;  offset  size  content
;     +0     2   segment of environment for child (0 = current)
;     +2     4   address of command line to place at PSP:0080
;     +6     4   address of an FCB to be placed at PSP:005c
;    +0Ah    4   address of an FCB to be placed at PSP:006c
EXEC_PARAM_REC db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   ; +5Dh

; Program to execute, preset by SvarCOM (128 bytes, ASCIIZ)  ; +6Bh
EXECPROG dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

; offset within EXECPROG for out and in filenames in case stdin or stdout
; needs to be redirected (0xffff=no redirection)
REDIR_OUTFIL dw 0xffff    ; +EBh
REDIR_INFIL dw 0xffff     ; +EDh
REDIR_OUTAPPEND dw 0      ; +EFh

skipsig:         ; +F1h

; set up CS=DS=SS and point SP to my private stack buffer
mov ax, cs
mov ds, ax
mov es, ax
mov ss, ax
mov sp, STACKPTR

; revert stdin/stdout redirections (if any) to their initial state
call REVERT_REDIR_IF_ANY

; redirect stdout if required
call REDIR_OUTFILE_IF_REQUIRED

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
mov [cs:EXECPROG], byte 0 ; do not run app again (+DS might have been changed)

jmp short skipsig      ; enforce valid ds/ss/etc (can be lost after int 21,4b)

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
mov [EXEC_PARAM_REC+2], word CMDTAIL
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
; terminated with \r) - a single 0x0A byte is passed so SvarCOM knows it is
; called as respawn (as opposed to being invoked as a normal application)
; this allows multiple copies of SvarCOM to stack upon each other.
CMDTAIL db 0x01, 0x0A, 0x0D

ERRLOAD db "ERR x, FAILED TO LOAD COMMAND.COM", 13, 10, '$'

; variables used to revert stdin/stdout to their initial state
OLD_STDOUT dw 0xffff
OLD_STDIN  dw 0xffff


; ****************************************************************************
; *** ROUTINES ***************************************************************
; ****************************************************************************

; ----------------------------------------------------------------------------
; revert stdin/stdout redirections (if any) to their initial state
; all memory accesses are CS-prefixes because this code may be called at
; times when DS is out of whack.
REVERT_REDIR_IF_ANY:
; is stdout redirected?
mov bx, [OLD_STDOUT]
cmp bx, 0xffff
je STDOUT_DONE
; revert the stdout handle (dst in BX already)
mov cx, 1        ; src handle (1=stdout)
mov ah, 0x46     ; redirect a handle
int 0x21
; close the old handle (still in bx)
mov ah, 0x3e
int 0x21
mov [OLD_STDOUT], word 0xffff ; mark stdout as "not redirected"
STDOUT_DONE:
ret
; ----------------------------------------------------------------------------


; ----------------------------------------------------------------------------
; redirect stdout if REDIR_OUTFIL points to something
REDIR_OUTFILE_IF_REQUIRED:
mov si, [REDIR_OUTFIL]
cmp si, 0xffff
je NO_STDOUT_REDIR
add si, EXECPROG       ; si=output file
mov ax, 0x6c00         ; Extended Open/Create
mov bx, 1              ; access mode (0=read, 1=write, 2=r+w)
xor cx, cx             ; file attribs when(if) file is created (0=normal)
mov dx, [REDIR_OUTAPPEND] ; action if file exist (0x11=open, 0x12=truncate)
int 0x21               ; ax=handle on success (CF clear)
mov [REDIR_OUTFIL], word 0xffff
jc NO_STDOUT_REDIR     ; TODO: abort with an error message instead

; jump to end of file if flag was 0x11 (required for >> redirections)
cmp [REDIR_OUTAPPEND], word 0x11
jne SKIP_JMPEOF
mov bx, ax
mov ax, 0x4202         ; jump to position EOF - CX:DX in handle BX
xor cx, cx
xor dx, dx
int 0x21
mov ax, bx             ; put my handle back in ax, as expected by later code
SKIP_JMPEOF:

; duplicate current stdout so I can revert it later
push ax                ; save my file handle in stack
mov ah, 0x45           ; duplicate file handle BX
mov bx, 1              ; 1=stdout
int 0x21               ; ax=new (duplicated) file handle
mov [OLD_STDOUT], ax   ; save the old handle in memory

; redirect stdout to my file
pop bx                 ; dst handle
mov cx, 1              ; src handle (1=stdout)
mov ah, 0x46           ; "redirect a handle"
int 0x21

; close the original file handle, I no longer need it
mov ah, 0x3e           ; close a file handle (handle in BX)
int 0x21
NO_STDOUT_REDIR:
ret
; ----------------------------------------------------------------------------

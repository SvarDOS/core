;
; rmod - resident module of the SvarCOM command interpreter (NASM code)
;
; Copyright (C) 2021-2024 Mateusz Viste
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
SIG4 dw 0x2019   ;  +6  acts also as a guardval to detect severe stack overflows

; Buffer used to remember previous command, when SvarCOM calls the buffered
; input service at INT 21h,AH=0x0A.
; This buffer is right before the stack, so in case of a stack overflow event
; (for example because of a "too ambitious" TSR) only this buffer is damaged,
; and can be invalidated without much harm. To detect such damage, SvarCOM's
; transient part is appending a signature at the end of the buffer.
INPUTBUF: times 132 db 0 ; 130 bytes for the input buffer + 2 for signature

; DOS int 21h functions that I use require at least 40 bytes of stack under
; DOS-C (FreeDOS) kernel, so here I reserve 64 bytes juste to be sure
STACKBUF db "XXX  SVARCOM RMOD BY MATEUSZ VISTE  XXXXXXXXXXXXXXXXXXXXXXXXXXXX"
STACKPTR dw 0

; offset of the COMSPEC variable in the environment block, 0 means "use
; boot drive". this value is patched by the transient part of COMMAND.COM
COMSPECPTR dw 0  ; +CEh

; fallback COMSPEC string used if no COMSPEC is present in the environment.
; drive is patched by the transient part of COMMAND.COM
COMSPECBOOT db "@:\COMMAND.COM", 0 ; +D0h

; exit code of last application
LEXCODE  db 0    ; +DFh

; ExecParamRec used by INT 21h, AX=4b00 (load and execute program), 14 bytes:
;  offset  size  content
;     +0     2   segment of environment for child (0 = current)
;     +2     4   address of command line to place at PSP:0080
;     +6     4   address of an FCB to be placed at PSP:005c
;    +0Ah    4   address of an FCB to be placed at PSP:006c
EXEC_PARAM_REC db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   ; +E0h

; Program to execute, preset by SvarCOM (128 bytes, ASCIIZ)
EXECPROG: times 128 db 0                                     ; +EEh

; File where stdin and stdout should be redirected (0 = no redirection)
REDIR_INFIL:     times 128 db 0     ; +16Eh
REDIR_OUTFIL:    times 128 db 0     ; +1EEh
REDIR_OUTAPPEND: dw 0               ; +26Eh
REDIR_DEL_STDIN: db 0               ; +270h  indicates that the stdin file
                                    ;        should be deleted (pipes). This
                                    ;        MUST contain the 1st char of
                                    ;        REDIR_INFIL!

EXEC_LH: db 0                       ; +271h  EXECPROG to be loaded high?
ORIG_UMBLINKSTATE: db 0             ; +272h
ORIG_ALLOCSTRAT: db 0               ; +273h
CTRLC_FLAG: db 0                    ; +274h  flag that says "aborted by CTRL+C"

; CTRL+BREAK (int 23h) handler
; According to the TechHelp! Manual: "If you want to abort (exit to the parent
; process), then set the carry flag and return via a FAR RET. This causes DOS
; to perform normal cleanup and exit to the parent." (otherwise use iret)
BREAK_HANDLER:            ; +275h
mov [CTRLC_FLAG], byte 1  ; checked by SvarCOM to abort BAT files
stc
retf

; INT 0x2E handler
INT2E:
xor ax, ax
iret

skipsig:                  ; +27Fh

; set up CS=DS=SS and point SP to my private stack buffer
mov ax, cs
mov ds, ax
mov es, ax
mov ss, ax
mov sp, STACKPTR

; set up myself as break handler (int 0x23)
mov ax, 0x2523  ; set int vector 23h
mov dx, BREAK_HANDLER
int 0x21

; set up myself as criterr handler (int 0x24)
mov ax, 0x2524
mov dx, HANDLER_24H
int 0x21

; set up myself as int 0x2E handler ("pass command to shell")
mov ax, 0x252E
mov dx, INT2E ; TODO do something meaningful instead of a no-op
int 0x21

; revert stdin/stdout redirections (if any) to their initial state
call REVERT_REDIR_IF_ANY

; redirect stdin and/or stdout if required
call REDIR_INOUTFILE_IF_REQUIRED

; should I execute command.com or a pre-set application?
cmp [EXECPROG], byte 0
jz EXEC_COMMAND_COM

; TODO: perhaps I should call the DOS SetPSP function here? But if I do, the
;       int 21h, ah=50h call freezes...
;mov ah, 0x50           ; DOS 2+ -- Set PSP
;mov bx, cs
;int 0x21


; LOADHIGH?
cmp [EXEC_LH], byte 0
je NO_LOADHIGH
; SAVE CURRENT UMB LINK STATE
mov ax, 0x5802  ; GET UMB LINK STATE
int 0x21
mov [ORIG_UMBLINKSTATE], al
; SAVE CURRENT ALLOCATION STRATEGY
mov ax, 0x5800
int 0x21
mov [ORIG_ALLOCSTRAT], al

; LOADHIGH: link in the UMB memory chain for enabling high-memory allocation
;           (and save initial status on stack)
mov ax, 0x5803  ; SET UMB LINK STATE */
mov bx, 1
int 0x21
; set strategy to 'last fit, try high then low memory'
mov ax, 0x5801
mov bx, 0x0082
int 0x21
NO_LOADHIGH:

; exec an application preset (by SvarCOM) in the ExecParamRec
mov ax, 0x4B00         ; DOS 2+ - load & execute program
mov dx, EXECPROG       ; DS:DX  - ASCIZ program name (preset at PSP[already)
mov bx, EXEC_PARAM_REC ; ES:BX  - parameter block pointer
int 0x21
mov [cs:EXECPROG], byte 0 ; do not run app again (+DS might have been changed)

; go to start if nothing else to do (this will enforce valid ds/ss/etc)
cmp [cs:EXEC_LH], byte 0
je skipsig

; restore UMB link state and alloc strategy to original values (but make sure
; to base it on CS since DS might have been trashed by the program)
mov ax, 0x5803
xor bx, bx
mov bl, [cs:ORIG_UMBLINKSTATE]
int 0x21
; restore original memory allocation strategy
mov ax, 0x5801
mov bl, [cs:ORIG_ALLOCSTRAT]
int 0x21
; turn off the LH flag
mov [cs:EXEC_LH], byte 0


jmp skipsig      ; enforce valid ds/ss/etc (can be lost after int 21,4b)

EXEC_COMMAND_COM:

; collect the exit code of previous application
mov ah, 0x4D
int 0x21
mov [LEXCODE], al

; zero out the exec param block (14 bytes)
mov al, 0              ; byte to write
mov cx, 14             ; how many times
mov di, EXEC_PARAM_REC ; ES:DI = destination
cld                    ; stosb must move forward
rep stosb              ; repeat cx times

; zero out the LH flag
mov [EXEC_LH], byte 0

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

; save error code into cl
mov cl, al

; display program name (in DS:DX), but first replace its nul terminator by a $
mov si, dx
PARSENEXTBYTE:
lodsb ; load byte at DS:SI into AL and inc SI (direction flag is clear already)
test al, al  ; is zero yet?
jnz PARSENEXTBYTE
mov [si], byte '$'   ; replace the nul terminator by a $ and display it
mov ah, 0x09
int 0x21
mov [si], byte 0     ; revert the nul terminator back to its place

; restore DS=CS
mov bx, cs
mov ds, bx

; update error string so it contains the error number
mov al, '0'
add al, cl    ; the exec error code
mov [ERRLOAD + 6], al

; display error message
mov ah, 0x09
mov dx, ERRLOAD
int 0x21

; wait for keypress
mov ah, 0x07    ; use INT 21h,AH=7 instead of AH=8 for CTRL+C immunity
int 0x21

; back to program start
jmp skipsig

; command.com tail arguments, in PSP format: length byte followed by args and
; terminated with \r) - a single 0x0A byte is passed so SvarCOM knows it is
; called as respawn (as opposed to being invoked as a normal application)
; this allows multiple copies of SvarCOM to stack upon each other.
CMDTAIL db 0x01, 0x0A, 0x0D

ERRLOAD db ": ERR x, LOAD FAILED", 0x0A, 0x0D, '$'

; variables used to revert stdin/stdout to their initial state
OLD_STDOUT dw 0xffff
OLD_STDIN  dw 0xffff


; ****************************************************************************
; *** ROUTINES ***************************************************************
; ****************************************************************************

; ----------------------------------------------------------------------------
; revert stdin/stdout redirections (if any) to their initial state
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

; is stdin redirected?
mov bx, [OLD_STDIN]
cmp bx, 0xffff
je STDIN_DONE
; revert the stdin handle (dst in BX already)
xor cx, cx       ; src handle (0=stdin)
mov ah, 0x46     ; redirect a handle
int 0x21
; close the old handle (still in bx)
mov ah, 0x3e
int 0x21
mov [OLD_STDIN], word 0xffff ; mark stdin as "not redirected"

; delete stdin file if required
cmp [REDIR_DEL_STDIN], byte 0
je STDIN_DONE
; revert the original file and delete it
mov ah, [REDIR_DEL_STDIN]
mov [REDIR_INFIL], ah
mov ah, 0x41     ; DOS 2+ - delete file pointed at by DS:DX
mov dx, REDIR_INFIL
int 0x21
mov [REDIR_INFIL], byte 0
mov [REDIR_DEL_STDIN], byte 0

STDIN_DONE:

ret
; ----------------------------------------------------------------------------


; ----------------------------------------------------------------------------
; redirect stdout if REDIR_OUTFIL points to something
REDIR_INOUTFILE_IF_REQUIRED:
cmp [REDIR_OUTFIL], byte 0
je NO_STDOUT_REDIR
mov si, REDIR_OUTFIL   ; si = output file
mov ax, 0x6c00         ; Extended Open/Create
mov bx, 1              ; access mode (0=read, 1=write, 2=r+w)
xor cx, cx             ; file attribs when(if) file is created (0=normal)
mov dx, [REDIR_OUTAPPEND] ; action if file exist (0x11=open, 0x12=truncate)
int 0x21               ; ax=handle on success (CF clear)
mov [REDIR_OUTFIL], byte 0
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
mov bx, 1              ; 1 = stdout
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

; *** redirect stdin if REDIR_INFIL points to something ***
cmp [REDIR_INFIL], byte 0
je NO_STDIN_REDIR
mov dx, REDIR_INFIL    ; dx:dx = file
mov ax, 0x3d00         ; open file for read
int 0x21               ; ax=handle on success (CF clear)
mov [REDIR_INFIL], byte 0
jc NO_STDIN_REDIR      ; TODO: abort with an error message instead

; duplicate current stdin so I can revert it later
push ax                ; save my file handle in stack
mov ah, 0x45           ; duplicate file handle BX
xor bx, bx             ; 0=stdin
int 0x21               ; ax=new (duplicated) file handle
mov [OLD_STDIN], ax    ; save the old handle in memory

; redirect stdout to my file
pop bx                 ; dst handle
xor cx, cx             ; src handle (0=stdin)
mov ah, 0x46           ; "redirect a handle"
int 0x21

; close the original file handle, I no longer need it
mov ah, 0x3e           ; close a file handle (handle in BX)
int 0x21
NO_STDIN_REDIR:
ret


; ****************************************************************************
; *                                                                          *
; * INT 24H HANDLER                                                          *
; *                                                                          *
; ****************************************************************************
;
; this is an executable image that can be set up as the critical error handler
; interrupt (int 24h). It displays the usual "Abort, Retry, Fail..." prompt.
;
; documentation:
; http://www.techhelpmanual.com/564-int_24h__critical_error_handler.html
;
; === CRIT HANDLER DETAILS ====================================================
;
; *** ON ENTRY ***
;
; upon entry to the INT 24h handler, the registers are as follows:
;  BP:SI = addr of a "device header" that identifies the failing device
;     DI = error code in lower 8 bits (only for non-disk errors)
;     AL = drive number, but only if AH bit 7 is reset
;     AH = error flags
;        0x80 = reset if device is a disk, set otherwise
;           all the following are valid ONLY for disks (0x80 reset):
;        0x20 = set if "ignore" action allowed
;        0x10 = set if "retry" action allowed
;        0x08 = set if "fail" action allowed
;        0x06 = disk area, 0=sys files, 1=fat, 10=directory, 11=data
;        0x01 = set if failure is a write, reset if read
;
; within the int 24h handler, only these DOS functions are allowed:
;   01H-0CH (DOS character I/O)
;   33H (all subfns are OK, including 3306H get DOS version)
;   50H (set PSP address)
;   51H and 62H (query PSP address)
;   59H (get extended error information)
;
; *** ON EXIT ***
;
; After handling the error, AL should be set with an action code and get back
; to DOS. Available actions are defined in AH at entry (see above). Possible
; values on exit are:
;   AL=0  ignore error (pretend nothing happened)
;   AL=1  retry operation
;   AL=2  abort (terminates the failed program via int 23h, like ctrl+break)
;   AL=3  return to application indicating a failure of the DOS function
;
; A very basic "always fail" handler would be as simple as this:
;   mov al, 3
;   iret
;
; *** DOS CALLS ***
;
; Warning! Be careful about using DOS fns in your Critical Error handler.
;          With DOS 5.0+, ONLY the following fns can be called safely:
;
;          01H-0CH (DOS character I/O)
;          33H (all subfns are OK, including 3306H get DOS version)
;          50H (set PSP address)
;          51H and 62H (query PSP address)
;          59H (get extended error information)
;
; =============================================================================
HANDLER_24H:    ; +46Ch

; save registers so I can restore them later
; this is not needed, DOS saves it already before calling the int 24h handler
;push ax
;push bx
;push cx
;push dx
;push ds
;pushf

; set DS to myself
push cs
pop ds

; is this a DISK error?
test ah, 0x80
jz DISKERROR
; non-disk error: output "CRITICAL ERROR #XXX SYSTEM HALTED" and freeze
; update the crit string so it contains the proper error code
mov bx, CRITERRSYST+1
mov ax, di
xor ah, ah
mov cl, 100
div cl ; AL = AX / cl     AH = remainder
add al, '0'
mov [bx], al
inc bx

mov al, ah
xor ah, ah
mov cl, 10
div cl
add al, '0'
add ah, '0'
mov [bx], al
inc bx
mov [bx], ah

; display the string
mov ah, 0x09
mov dx, CRITERR
int 0x21
mov dx, CRITERRSYST
int 0x21
; freeze the system
HALT:
sti
hlt;
jmp HALT

DISKERROR:
; disk errors produce this message:
; A: - READ|WRITE FAILURE
; (A)bort, (R)etry, (I)gnore, (F)ail
mov ch, ah      ; backup AH flags into CH
add al, 'A'
mov [CRITERRDISK], al
mov ah, 0x09
mov dx, CRITERRDISK
int 0x21
; READ / WRITE (test flag 0x01, set = write, read otherwise)
mov dx, CRITERRDSK_WRITE
test ch, 1
jnz WRITE
mov dx, CRITERRDSK_READ
WRITE:
int 0x21
mov dx, CRLF
int 0x21
; print available options (abort=always, 0x10=retry, 0x20=ignore, 0x08=fail)
CRITERR_ASKCHOICE:
mov dx, CRITERR_ABORT
int 0x21
; retry?
test ch, 0x10
jz NORETRY
mov dx, CRITERR_COMMA
int 0x21
mov dx, CRITERR_RETRY
int 0x21
NORETRY:
; ignore?
test ch, 0x20
jz NOIGNORE
mov dx, CRITERR_COMMA
int 0x21
mov dx, CRITERR_IGNOR
int 0x21
NOIGNORE:
; fail?
test ch, 0x08
jz NOFAIL
mov dx, CRITERR_COMMA
int 0x21
mov dx, CRITERR_FAIL
int 0x21
NOFAIL:
; output "? "
mov ah, 0x06
mov dl, '?'
int 0x21
mov dl, ' '
int 0x21

; wait for user key press and return (iret) with AL set accordingly:
;   AL=0  ignore
;   AL=1  retry
;   AL=2  abort
;   AL=3  fail

mov ah, 0x07  ; key input, no ctrl+c check
int 0x21
and al, 0xdf   ; switch AL to upper-case so key check is case-insensitive
mov cl, al     ; save pressed key in CL (AL is easily overwritten by DOS calls)
; print the pressed key
mov ah, 0x06
mov dl, cl
int 0x21
mov ah, 0x09
mov dx, CRLF
int 0x21

; was it abort?
cmp cl, [CRITERR_KEYS]
jne SKIP_ABORT
mov al, 2     ; AL=2 -> "abort"
iret
SKIP_ABORT:
; if retry is allowed - was it retry?
test ch, 0x10
jz SKIP_RETRY
cmp cl, [CRITERR_KEYS+1]
jne SKIP_RETRY
mov al, 1     ; AL=1 -> "retry"
iret
SKIP_RETRY:
; if ignore is allowed - was it ignore?
test ch, 0x20
jz SKIP_IGN
cmp cl, [CRITERR_KEYS+2]
jne SKIP_IGN
xor al, al    ; AL=0 -> "ignore"
iret
SKIP_IGN:
; if fail is allowed - was it fail?
test ch, 0x08
;jz SKIP_FAIL
cmp cl, [CRITERR_KEYS+3]
jne SKIP_FAIL
mov al, 3     ; AL=3 -> "fail"
iret
SKIP_FAIL:

jmp CRITERR_ASKCHOICE   ; invalid answer -> ask again

; restore registers and quit the handler
;popf
;pop ds
;pop dx
;pop cx
;pop bx
;pop ax


CRITERR db "CRITICAL ERROR $"
CRITERRSYST db "#XXX - SYSTEM HALTED$"
CRITERRDISK db "@: - $"
CRITERRDSK_READ db "READ FAILURE$"
CRITERRDSK_WRITE db "WRITE FAILURE$"
CRLF db 0x0A, 0x0D, "$"
CRITERR_ABORT db "(A)bort$"
CRITERR_RETRY db "(R)etry$"
CRITERR_IGNOR db "(I)gnore$"
CRITERR_FAIL  db "(F)ail$"
CRITERR_KEYS  db "ARIF"
CRITERR_COMMA db ", $"

;
; WORK IN PROGRESS TEMPLATE! NOT USED YET
;
; int 24h handler, part of SvarCOM. MIT license.
; Copyright (C) 2022 Mateusz Viste
;
; this is an executable image that can be set up as the critical error handler
; interrupt (int 24h). It displays the usual "Abort, retry, fail" prompt.
;
; documentation:
; http://www.techhelpmanual.com/564-int_24h__critical_error_handler.html
;

org 0   ; this code does not have a PSP, it is loaded as-is into a memory
        ; segment

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
;
; *** ON EXIT ***
;
; After handling the error, AL should be set with an action code and get back
; to DOS. Available actions are defined in AH at entry (see above). Possible
; values on exit are:
;   AL=0  ignore error (pretend nothing happenned)
;   AL=1  retry operation
;   AL=2  abort (terminates the failed program via int 23h, like ctrl+break)
;   AL=3  return to application indicating a failure of the DOS function
;
; A very basic "always fail" handler would be as simple as this:
;   mov al, 3
;   iret
;
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
; =============================================================================


; save registers so I can restore them later
push ah
push bx
push cx
push dx
pushf

; disk errors product this message:
; CRITICAL ERROR - DRIVE A: - READ|WRITE FAILURE
; (A)bort, (R)etry, (F)ail
;
; non-disk errors produce this:
; CRITICAL ERROR #errno


; restore registers and quit the handler
popf
pop dx
pop cx
pop bx
pop ah

iret


; write DX string to stderr
write_dx_str_to_stderr:
push ax
push bx
push cx

mov ah, 0x40       ; write to file -- am I sure I can use this function safely?
mov bx, 2          ; stderr
mov cx, 1          ; one byte
int 0x21

pop cx
pop bx
pop ax

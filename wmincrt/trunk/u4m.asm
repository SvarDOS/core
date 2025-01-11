

; 32 bit unsigned multiplication
;   IN : DXAX=first operand, CXBX=second operand
;   OUT: DXAX=result (maybe truncated)
;   MOD: BX,CX

include "common.inc"

startcode

pubproc __U4I
pubproc __U4M
      push si   ; SI: save AX
      push di   ; DI: temp result high

      ; test if we can do a 16-bit multiplication
      mov si,ax
      mov di,cx
      or di,dx
      jz @@U2M
      ; do the full 32-bit multiplication
      mov ax,bx
      mul dx
      mov di,ax
      mov ax,si
      mul cx
      add di,ax
      mov ax,si
@@U2M:mul bx
      add dx,di

      pop di
      pop si
      ret
endproc __U4M
endproc __U4I

endcode

end

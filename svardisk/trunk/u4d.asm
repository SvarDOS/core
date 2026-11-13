; 32-bit unsigned divide for the 8086.
; Watcom calls __U4D with the dividend in DX:AX and the divisor in CX:BX.
; The quotient comes back in DX:AX and the remainder in CX:BX.
; SI, DI and BP are preserved. A zero divisor yields a quotient of all ones.

.8086

include "common.inc"

startcode

pubproc __U4D
      push bp
      mov bp,sp
      push si
      push di

      mov si,bx
      or si,cx
      jnz @div_ok
      mov ax,0ffffh
      mov dx,ax
      xor bx,bx
      xor cx,cx
      jmp @div_out

@div_ok:
      or cx,cx
      jnz @div_full
      or dx,dx
      jnz @div_32_16
      cmp ax,bx
      jb @div_small
      div bx
      mov bx,dx
      xor dx,dx
      xor cx,cx
      jmp @div_out

@div_small:
      mov bx,ax
      xor ax,ax
      xor dx,dx
      xor cx,cx
      jmp @div_out

@div_32_16:
      push ax
      mov ax,dx
      xor dx,dx
      div bx
      mov si,ax
      pop ax
      div bx
      mov bx,dx
      xor cx,cx
      mov dx,si
      jmp @div_out

@div_full:
      push bx
      push cx
      xor si,si
      xor di,di
      mov cx,32
@div_bit:
      shl ax,1
      rcl dx,1
      rcl di,1
      rcl si,1
      cmp si,[bp-8]
      ja @div_sub
      jb @div_nosub
      cmp di,[bp-6]
      jb @div_nosub
@div_sub:
      sub di,[bp-6]
      sbb si,[bp-8]
      or ax,1
@div_nosub:
      loop @div_bit
      add sp,4
      mov bx,di
      mov cx,si

@div_out:
      pop di
      pop si
      pop bp
      ret
endproc __U4D

endcode

end

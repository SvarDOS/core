include "common.inc"

startdata
      extrn _crt_stk_bottom:word
enddata

startcode
      extrn crt_panic_:near

pubproc __STK


      ; ensures that we have enough stack space left. the needed bytes are
      ; given in AX. panics if stack low.
      sub ax,sp         ; subtract needed bytes from SP
      neg ax            ; SP-AX = -(AX-SP)
      add ax,2          ; +2 is to compensate for __STK ret addr
      cmp ax,[_crt_stk_bottom]
      jae @stk_stat     ; enough stack => return, else panic
  IFDEF DEBUG
      int 3             ; give debugger chance to trap
  ENDIF DEBUG
      push cs
      pop ds
      mov dx,offset stkerr_msg
      add sp,200h       ; make sure we have enough stack to call DOS
      jmp crt_panic_
@stk_stat:
  IFDEF STACKSTAT       ; update lowest stack pointer if statistics enabled
    _BSS segment

      public _crt_stack_low

      _crt_stack_low:  dw 1 dup(?)
    _BSS ends

      cmp [_crt_stack_low],ax
      jbe @stk_r
      mov [_crt_stack_low],ax
  ENDIF STACKSTAT
@stk_r:
      ret
      stkerr_msg db 'STKERR',13,10,'$'
endproc __STK

endcode

end


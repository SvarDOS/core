;-----------------------------------------------------------------------------
; ALLOCATION STACK ROUTINES

include "common.inc"


startcode

startdata
      extrn _crt_stk_bottom:word
enddata

pubproc crt_stk_mem_alloc_
      push dx
      add ax,[_crt_stk_bottom]
      jc @alloc_err                 ; we do not want to overflow!
      mov dx,ax
      add ax,STK_ALLOC_MARGIN
      jc @alloc_err                 ; we do not want to overflow!
      cmp ax,sp
      jae @alloc_err
      mov ax,dx
      mov [_crt_stk_bottom],ax
      pop dx
      ret
@alloc_err:
      pop dx
      xor ax,ax
      ret
endproc crt_stk_mem_alloc_

pubproc crt_stk_mem_rewind_
      mov [_crt_stk_bottom],ax
      ret
endproc crt_stk_mem_rewind_

endcode

end

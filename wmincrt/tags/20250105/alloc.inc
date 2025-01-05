;-----------------------------------------------------------------------------
; ALLOCATION STACK ROUTINES

  public crt_stk_mem_alloc_
  public crt_stk_mem_rewind_

crt_stk_mem_alloc_ proc
      push dx
      add ax,[stk_bottom]
      jc @alloc_err                 ; we do not want to overflow!
      mov dx,ax
      add ax,STK_ALLOC_MARGIN
      jc @alloc_err                 ; we do not want to overflow!
      cmp ax,sp
      jae @alloc_err
      mov ax,dx
      mov [stk_bottom],ax
      pop dx
      ret
@alloc_err:
      pop dx
      xor ax,ax
      ret
crt_stk_mem_alloc_ endp

crt_stk_mem_rewind_ proc
      mov [stk_bottom],ax
      ret
crt_stk_mem_rewind_ endp

IFNDEF NOARGV

      ; We process the command line given to us by the PSP into the
      ; __argc and __argv variables.

      ; First byte of _crt_cmdline contains the length of the command line.
      ; The cmdline provided to us by the PSP may or may not be terminated
      ; by a carriage return (0ch).
      ; It may be 7fh bytes at max. If it is 7fh bytes, there is no space
      ; for a terminating zero, therefore we error out if it is equal or more
      ; than 7fh bytes long.

      ; Whitespace between arguments is filled with zero, which implicitly
      ; zero-terminates all arguments but the last.

      ; We consider every character below and including 20h (space)
      ; as whitespace. 0Ch (CR) terminates the command line.

  _DATA segment
      public __argc
      public _crt_argc

    _crt_argc label word
    __argc dw 1          ; number or arguments, first is program name

    prg_name db 'NONAME',0
  _DATA ends

  _BSS segment
      public __argv
      public _crt_argv

    _crt_argv label word
    __argv dw ?          ; pointer to array of cmdline argument pointers
  _BSS ends

      xor bx,bx                     ; use bx as zero store
      ; We place to argv pointer array on the bottom of the stack.
      ; We loose a few bytes of stack space, but that should be ok.
      mov di,offset DGROUP:_STACK
      ; ARGV[0] is typically the name of the executable, currently set to NULL
      ; TODO: implement properly
      mov [__argv],di
      mov [di],offset DGROUP:prg_name
      inc di
      inc di
      ; load cmdline length and verify we can deal with it
      mov si,[_crt_cmdline]
      lodsb                         ; load cmdline length should be 7fh at max                
      mov cl,al
      xor dh,dh
      cmp cl,07fh                   ; make sure there is space for trailing zero
      jae @arg_done                 ; or else return empty argv
@arg_skip_ws:
      ; Skip over whitespace characters while replacing them by zeroes.
      ; This impliciyly zero-terminates the preceeding argument.
      mov al,[si]                   ; get next chararcter
      cmp al,' '                    ; is it a whitespace?
      ja @arg_start                 ; if not, argument starts
      cmp al,13                     ; CR terminates the command line
      je @arg_done
      mov [si],bl                   ; replace whitespace with zero
      dec cx                        ; one character less
      jcxz @arg_done                ; no more characters?
      inc si                        ; next character
      jmp @arg_skip_ws              ; contine whitespace skipping
@arg_start:
      inc [__argc]
      mov [di],si                   ; store next ARGV pointer
      inc di
      inc di
@arg_loop:
      mov al,[si]                   ; get next character
      cmp al,' '                    ; is it a whitespace?
      jbe @arg_skip_ws              ; if yes, skip over it
      inc si
      loop @arg_loop

@arg_done:
      mov [si],bl                   ; zero-terminate last argument
      mov word ptr [di],bx          ; zero-terminate argv ptr array

      mov ax,[__argc]
      mov dx,[__argv]
ENDIF

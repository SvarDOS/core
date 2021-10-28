/*
 * type
 */

static int cmd_type(struct cmd_funcparam *p) {
  char *buff = p->BUFFER;
  const char *fname = p->argv[0];
  unsigned short err = 0;

  if (p->argc == 0) {
    outputnl("Required parameter missing");
    return(-1);
  }

  if ((p->argc > 0) && (imatch(p->argv[0], "/?"))) {
    outputnl("Displays the contents of a text file.");
    outputnl("");
    outputnl("TYPE [drive:][path]filename");
    return(-1);
  }

  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(-1);
  }

  /* if here then display the file */
  _asm {
    push ax
    push bx
    push cx
    push dx
    push si

    mov ax, 0x3d00 /* open file via handle, access mode in AL (0 = read) */
    mov dx, fname
    int 0x21       /* file handle in ax on success (CF clear) */
    jnc FILE_OPEN_OK
    mov [err], ax  /* on error AX contains the DOS err code */
    jmp FOPENFAIL
    FILE_OPEN_OK:
    /* copy obtained file handle to BX */
    mov bx, ax

    READNEXTBLOCK:
    /* read file block by block */
    mov cx, 1024   /* read 1K at a time */
    mov dx, buff
    mov ah, 0x3f   /* read CX bytes from file handle in BX and write to DS:DX */
    int 0x21       /* CF set on error, AX=errno or AX=number of bytes read */
    jc GOTERROR    /* abort on error */
    test ax, ax    /* EOF? */
    jz ENDFILE
    /* display read block (AX=len) */
    mov si, dx     /* preset DS:SI to DS:DX (DL will be reused soon) */
    mov cx, ax     /* set loop count to CX */
    mov ah, 0x02   /* write character in DL to stdout */
    NEXTCHAR:
    mov dl, [si]
    inc si
    int 0x21
    loopnz NEXTCHAR /* CX-- ; jnz NEXTCHAR (display CX characters) */
    /* read (and display) next block */
    jmp READNEXTBLOCK

    GOTERROR:
    mov [err], ax

    ENDFILE:
    /* close file */
    mov ah, 0x3e   /* close file handle (file handle already in BX) */
    int 0x21

    FOPENFAIL:

    pop si
    pop dx
    pop cx
    pop bx
    pop ax
  }

  if (err != 0) outputnl(doserr(err));

  return(-1);
}

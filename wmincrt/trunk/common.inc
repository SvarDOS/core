
include "config.inc"
include "segments.inc"

startcode macro
_TEXT segment
endm

endcode macro
_TEXT ends
endm

startdata macro
_DATA segment
endm

enddata macro
_DATA ends
endm

startbss macro
_BSS segment
endm

endbss macro
_BSS ends
endm

startconst macro
CONST segment
endm

endconst macro
CONST ends
endm

pubproc macro p
    public p
p proc
endm

endproc macro p
p endp
endm


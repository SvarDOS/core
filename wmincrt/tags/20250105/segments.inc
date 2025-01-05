; SEGMENT DEFINITIONS

IFDEF EXE
  DGROUP group _NULL,_AFTERNULL,CONST,CONST2,STRINGS,_DATA,_BSS,_STACK
ELSE EXE
  DGROUP group _TEXT,_DATA,CONST,CONST2,_BSS,_STACK
ENDIF EXE


IFDEF EXE
  BEGTEXT segment word public 'CODE'
  BEGTEXT ends
ENDIF EXE

_TEXT segment word public 'CODE'
_TEXT ends

IFDEF EXE
  FAR_DATA segment byte public 'FAR_DATA'
  FAR_DATA ends

  _NULL   segment para public 'BEGDATA'
  _NULL   ends

  _AFTERNULL segment word public 'BEGDATA'
  _AFTERNULL ends
ENDIF EXE

CONST segment word public 'DATA'
CONST ends

CONST2 segment word public 'DATA'
CONST2 ends

IFDEF EXE
  STRINGS segment word public 'DATA'
  STRINGS ends
ENDIF EXE

_DATA segment word public 'DATA'
_DATA ends

_BSS segment word public 'BSS'
_BSS  ends

IFDEF EXE
  _STACK segment para public 'STACK'
  _STACK ends
ELSE EXE
  _STACK segment para public 'TINY_STACK'
  _STACK ends
ENDIF EXE

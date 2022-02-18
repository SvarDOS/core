@ECHO OFF
PROMPT $P$G

REM DISPLAY driver for NLS support (if display.exe exists)
IF EXIST DISPLAY.EXE DISPLAY CON=(EGA,,1)

FDAPM APMDOS

ECHO.
ECHO  **************************
ECHO  *** Welcome to SvarDOS ***
ECHO  **************************
ECHO.

INSTALL

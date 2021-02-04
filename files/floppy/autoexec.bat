@ECHO OFF
PATH A:\
SET NLSPATH=A:\
PROMPT $P$G
SET DIRCMD=/P /OGNE /4
ALIAS REBOOT=FDAPM COLDBOOT
ALIAS HALT=FDAPM POWEROFF

REM DISPLAY driver for NLS support
DISPLAY CON=(EGA,,1)

FDAPM APMDOS

ECHO.
ECHO  **************************
ECHO  *** Welcome to SvarDOS ***
ECHO  **************************
ECHO.

INSTALL

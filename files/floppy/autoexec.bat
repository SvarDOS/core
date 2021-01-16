@ECHO OFF
PATH A:\
SET NLSPATH=A:\
PROMPT $P$G
SET DIRCMD=/P /OGNE /4
ALIAS REBOOT=FDAPM COLDBOOT
ALIAS HALT=FDAPM POWEROFF

REM DISPLAY driver for NLS support
DISPLAY CON=(EGA,,1)

REM *** CDROM initialization - ONLY IF REQUIRED FOR PACKAGES INSTALL ***

IF EXIST A:\ATTRIB.ZIP GOTO SKIPCDINIT

REM UDVD2 is called with /UX to DISABLE UltroDMA (force PIO mode). Otherwise
REM freezes happen when running VirtualBox (tested under vbox 5.0 and 5.1)
IF NOT EXIST SVCD0001 DEVLOAD /Q /H A:\UDVD2.SYS /D:SVCD0001 /UX

REM if UDVD2 failed, try using eltorito as last resort
REM under Virtualbox 6.0 eltorito leads to an empty drive
IF NOT EXIST SVCD0001 DEVLOAD /Q /H A:\ELTORITO.SYS /D:SVCD0001

REM enable the CD only if driver succeeded
IF EXIST SVCD0001 SHSUCDX /D:SVCD0001 /Q

:SKIPCDINIT

FDAPM APMDOS

ECHO.
ECHO  **************************
ECHO  *** Welcome to SvarDOS ***
ECHO  **************************
ECHO.

INSTALL

@echo off

REM This batch file expects to be run in the tree directory.
REM And it expects compiler CC [bcc32] to be in the path.
REM It also expects Borland's Include and Lib directories to be set
REM via the enviroment variables INCLUDE and LIB respectively.
REM eg:
REM set INCLUDE=C:\Borland\bcc55\Include
REM set LIB=C:\Borland\bcc55\Lib;C:\Borland\bcc55\Lib\PSDK
REM for easier cleanup use BIN as output directory

if %1'==' goto badArgument

REM check if CC set, otherwise use default Borland's bcc32
if NOT %CC32%'==' goto usecats
set CC32=bcc32

:usecats
REM if adding cats add necessary files to compile command line
if NOT %1==CATS goto checkDefDrive
set TREECMPLXDEF=-DUSE_CATGETS
set TREECMPLXTRA=catgets.c db.c get_line.c

:checkDefDrive
if NOT %BORLANDINSTDRV%'==' goto checkInclude
set BORLANDINSTDRV=C:

:checkInclude
if NOT "%INCLUDE%"=="" goto checkLib
set INCLUDE=%BORLANDINSTDRV%\Borland\bcc55\Include

:checkLib
if NOT "%LIB%"=="" goto checkStub
set LIB=%BORLANDINSTDRV%\Borland\bcc55\Lib;%BORLANDINSTDRV%\Borland\bcc55\Lib\PSDK

:checkStub
REM note the %2 on the end is use to specify the name of the DOS STUB program.
REM if %2 is blank then it is assumed the default DOS stub should be used
REM i.e. the normal 'This program must run under Windows' message is displayed.
if %2'==' goto nostub
echo STUB '%2' > tree.def
:nostub

:compileDOS
md BIN > \NUL
%CC32% -DWIN32 -I%INCLUDE% -L%LIB% %TREECMPLXDEF% -nBIN -tWC -etree.exe tree.cpp stack.c %TREECMPLXTRA% user32.lib
copy BIN\tree.exe . > \NUL

:cleanup
echo cleaning up
set TREECMPLXTRA=
set TREECMPLXDEF=
REM set BORLANDINSTDRV=
REM copy tree.exe to current directory
del BIN\*.* < YES.TXT > \NUL
rd BIN > \NUL
if exist tree.def del tree.def
@echo Tree for Windows is now compiled. [tree.exe]
goto done

:badArgument
REM display proper usage
echo Usage: makedos CATS/NOCATS
echo where CATS specifies compile with Cats support and any thing
echo   else means do not use cats.  NOTE: compiling with cats
echo   will cause the resulting executable (NOT source) to be
echo   under the LGPL as the cats files are LGPL and not PD.
echo.
echo It expects Borland's Include and Lib directories to be set
echo   via the enviroment variables INCLUDE and LIB respectively.
echo.
echo The default compiler is bcc32, but the environment variable CC may
echo   be set before calling makwinbc.bat to use a different compiler
echo   that is compatible with bcc32.
echo.
echo This batch file must be ran from the tree\ directory.
echo Try again.

:done

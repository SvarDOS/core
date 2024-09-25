@echo off
echo Compiling Win32 version of tree using MS VC.

REM This batch file expects to be run in the tree directory.
REM And it expects Visual C/C++ (cl) to be in the path.

REM Make sure necessary args are specified
if %1'==' goto badargs

REM for easier cleanup uses BIN as output directory
md BIN > \NUL

REM Change cl and options if you are using a compiler not
REM compatible with Microsoft Visual C/C++ version 5

REM The default options are specified here, change this if necessary
set TREECMPLOPTS=/Fe"tree.exe" /nologo /ML /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"BIN/"

REM %1 is used to specify if cats support should be added (%1==CATS) or not (%1!=CATS)
if NOT %1==CATS goto skipcats
set TREECMPLOPTS=%TREECMPLOPTS% /D USE_CATGETS catgets.c db.c get_line.c
:skipcats

REM add default files and libraries to link
set TREECMPLOPTS=%TREECMPLOPTS% tree.cpp stack.c /link user32.lib

REM note the %2 on the end is use to specify the name of the DOS STUB program.
REM if %2 is blank then it is assumed the default DOS stub should be used
REM i.e. the normal 'This program must run under Windows' message is displayed.
if %2'==' goto nostub
echo Ignore the warning about stub file missing full MS-DOS header.
set TREECMPLOPTS=%TREECMPLOPTS% /STUB:%2
:nostub

REM actually perform compile
cl %TREECMPLOPTS%

:cleanup
echo cleaning up
REM unset env variable
set TREECMPLOPTS=
REM copy tree.exe to current directory
copy BIN\tree.exe . > \NUL
REM cleanup (remove) temp files
del BIN\*.* < YES.TXT > \NUL
rd BIN > \NUL
@echo Tree for Win32 Console now compiled. [tree.exe]
goto done

:badargs
echo USAGE: makwinvc CATS [stub] OR makwinvc NOCATS [stub]
echo where stub is the optional DOS stub to link with

:done

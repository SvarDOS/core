@echo off

REM This batch file expects to be run in the tree directory.
REM And it expects compiler CC [tcc or bcc] to be in the path.
REM for easier cleanup use BIN as output directory

if %1'==' goto badArgument
if %2'==' goto badArgument
if %3'==' goto badArgument

REM Setup the memory model to use and copy the matching file
set MEMMODEL=%2

REM check if CC set, otherwise use default Borland's tcc
if NOT %CC%'==' goto usecats
REM Change tcc to bcc if you are using Borland C/C++ 3.1
set CC=tcc

:usecats
REM if adding cats add necessary files to compile command line
if NOT %1==CATS goto compileDOS
set TREECMPLXDEF=-DUSE_CATGETS
set TREECMPLXTRA=catgets.c db.c get_line.c

:compileDOS
md BIN > \NUL
echo tree.cpp stack.c w32fDOS\%3\w32fdos.cpp w32fDOS\common\w32api.cpp %TREECMPLXTRA% > BIN\filelist
%CC% -DDOS -Iw32fDOS\common %TREECMPLXDEF% %MEMMODEL% -nBIN -tDe -etree.exe @BIN\filelist
copy BIN\tree.exe . > \NUL

:cleanup
echo cleaning up
set MEMMODEL=
set TREECMPLXTRA=
set TREECMPLXDEF=
REM copy tree.exe to current directory
del BIN\*.* < YES.TXT > \NUL
rd BIN > \NUL
@echo Tree for DOS is now compiled. [tree.exe]
goto done

:badArgument
REM display proper usage
echo Usage: makedos CATS/NOCATS -ml/-ms/other large/small/other
echo where CATS specifies compile with Cats support and any thing
echo   else means do not use cats.  NOTE: compiling with cats
echo   will cause the resulting executable (NOT source) to be
echo   under the LGPL as the cats files are LGPL and not PD.
echo and the second argument specifies the DOS memory model to use,
echo   this may be anything valid for the compiler,
echo   currently -ml for large model is recommended,
echo   but -ms may be used for small model; all other models untested.
echo The third argument specifies the subdirectory under w32fDOS for
echo   the file w32fDOS.cpp that matches the memory model specified.
echo   The default is 'large' when -ml is specified and 'small' for -ms.
echo.
echo If bcc is to be used (or another compiler with compatible arguments)
echo   then set the environment variable CC to bcc before calling makedos.bat.
echo.
echo This batch file must be ran from the tree\ directory.
echo Try again.

:done

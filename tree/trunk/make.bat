@echo off
echo This will create a version of Tree that can run under
echo Windows or DOS (in a single executable).
echo This batch file expects to be run in the tree directory.
echo It also expects the compilers to be in the path env variable.
echo If your compilers are not in your path, please set the path and 
echo rerun make.

if %1'==' goto badArgument
if %2'==' goto badArgument
if %3'==' goto badArgument

pause

:compileDOS
call makedos.bat %1 %2 %3
REM ren tree.exe treed.exe
REM compress so is smaller, apack seems better but only tested with upx
if %4'==UPX' upx --best --8086 -v tree.exe
REM important, must update so relocation table is moved to at least 0x40.
extra\fixstub tree.exe treed.exe
del tree.exe
REM fall through to vc, if you want bcc uncomment goto
REM goto compileWin32BCC

:compileWin32VC
call makwinvc.bat %1 treed.exe
del treed.exe
REM compress so is smaller, only tested with upx
if %4'==UPX' upx --best -v tree.exe
goto done

:compileWin32BCC
call makwinbc.bat %1 treed.exe
del treed.exe
REM compress so is smaller, only tested with upx
REM upx --best -v tree.exe
goto done

:badArgument
echo Usage: make CATS/NOCATS -ml/-ms/other large/small/other
echo where CATS/NOCATS determines if cats supported is compiled in
echo and -ml/-ms/other specifies memory model, with large/small/other
echo specifying the subdirectory under w32fDOS to copy w32fDOS.cpp
echo from (it should match the memory model specified).
echo The result of this batch file is a win32 console mode version
echo of tree with the DOS stub being a DOS version of tree.
echo The resulting executable should properly run under DOS or Win32.
echo Please try again.

:done
@echo Done.

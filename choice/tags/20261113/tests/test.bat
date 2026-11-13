@echo off
REM CHOICE compatibility checks. Run from this directory, or with CHOICE.COM
REM next to this file's parent. Errorlevel of this batch is 0 on success.

if "%1"=="" goto setbin
set CHOICE=%1
goto run
:setbin
set CHOICE=..\choice.com
if not exist %CHOICE% set CHOICE=choice.com

:run
set LANG=
set FAIL=0

echo --- no args, Y is 1
echo Y| %CHOICE% > out.tmp
if errorlevel 2 goto bad
if not errorlevel 1 goto bad

echo --- lowercase y is 1
echo y| %CHOICE% > out.tmp
if errorlevel 2 goto bad
if not errorlevel 1 goto bad

echo --- N is 2
echo N| %CHOICE% > out.tmp
if errorlevel 3 goto bad
if not errorlevel 2 goto bad

echo --- /N hides the list, n is 2
echo n| %CHOICE% /n > out.tmp
if errorlevel 3 goto bad
if not errorlevel 2 goto bad

echo --- /C:ABCDE E is 5
echo E| %CHOICE% /c:ABCDE > out.tmp
if errorlevel 6 goto bad
if not errorlevel 5 goto bad

echo --- /S keeps yn lowercase: Y beeps, y is 1
echo Yy| %CHOICE% /s > out.tmp
if errorlevel 2 goto bad
if not errorlevel 1 goto bad

echo --- empty /C
%CHOICE% /c > out.tmp
if not errorlevel 255 goto bad

echo --- empty /T
%CHOICE% /t > out.tmp
if not errorlevel 255 goto bad

echo --- timeout key not in the list
%CHOICE% /t:z,3 > out.tmp
if not errorlevel 255 goto bad

echo --- /? is 255
%CHOICE% /? > out.tmp
if not errorlevel 255 goto bad

echo --- unknown switch is 255
%CHOICE% /z > out.tmp
if not errorlevel 255 goto bad

echo --- prompt is glued to the bracket
echo N| %CHOICE% Testing this > out.tmp
if errorlevel 3 goto bad
if not errorlevel 2 goto bad
find "Testing this[Y,N]?" out.tmp > nul
if errorlevel 1 goto bad

echo --- quotes keep slashes, /N hides the list
echo n| %CHOICE% /n "Testing this /S /Z /T" > out.tmp
if errorlevel 3 goto bad
if not errorlevel 2 goto bad
find "Testing this /S /Z /T" out.tmp > nul
if errorlevel 1 goto bad

echo --- two prompts
echo n| %CHOICE% "Testing this" OK > out.tmp
if not errorlevel 255 goto bad

echo --- /T:n,0 selects N at once
%CHOICE% /t:n,0 > out.tmp
if errorlevel 3 goto bad
if not errorlevel 2 goto bad
find "[Y,N]?N" out.tmp > nul
if errorlevel 1 goto bad

echo --- glued switches /N/S and lowercase default
echo y| %CHOICE% /n/s > out.tmp
if errorlevel 2 goto bad
if not errorlevel 1 goto bad

echo --- last /C wins
echo B| %CHOICE% /c:yn /c:ab > out.tmp
if errorlevel 3 goto bad
if not errorlevel 2 goto bad

if exist choice.lng goto polish
if exist ..\choice.lng goto polish
goto nopolish
:polish
echo --- LANG=PL default keys are T/N
set LANG=PL
echo t| %CHOICE% > out.tmp
if errorlevel 2 goto bad
if not errorlevel 1 goto bad
find "[T,N]?" out.tmp > nul
if errorlevel 1 goto bad
set LANG=
:nopolish

del out.tmp
echo ALL TESTS PASSED
goto end

:bad
echo FAILED
set FAIL=1
del out.tmp

:end
if not "%FAIL%"=="0" exit 1
exit 0

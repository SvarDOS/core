@echo off

if not exist tmp\nul mkdir tmp

REM reuse FDNPKG code
copy /y ..\crc32.c tmp\
copy /y ..\fileexst.c tmp\
copy /y ..\getdelim.c tmp\
copy /y ..\helpers.c tmp\
copy /y ..\inf.c tmp\
copy /y ..\libunzip.c tmp\
copy /y ..\loadconf.c tmp\
copy /y ..\lsm.c tmp\
copy /y ..\parsecmd.c tmp\
copy /y ..\pkginst.c tmp\
copy /y ..\pkgrem.c tmp\
copy /y ..\readenv.c tmp\
copy /y ..\rtrim.c tmp\
copy /y ..\showinst.c tmp\

REM add a few fdinst-specific things
copy /y fdinst.c tmp\
copy /y kprintf0.c tmp\

wcl -0 -lr -ml -os -wx -we -d0 -i=..\ -i=..\zlib\ -DNOREPOS -DNOLZMA -fe=fdinst.exe tmp\*.c ..\zlib\zlib_l.lib
if ERRORLEVEL 1 goto gameover

del tmp\*.c
rmdir tmp

upx --8086 -9 fdinst.exe

:gameover

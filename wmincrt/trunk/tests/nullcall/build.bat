set PRGNAME=nullcall
wcc -q -d2 %PRGNAME%.c
wlink system dos debug all option quiet,eliminate,map libfile ..\..\lib\dbgexe\wmincrt.lib file %PRGNAME%

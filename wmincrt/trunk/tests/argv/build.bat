set PRGNAME=argv
wcc -q -ox %PRGNAME%.c
wlink system dos option quiet,eliminate,map libfile ..\..\lib\exe\wmincrt.lib file %PRGNAME%

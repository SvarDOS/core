set PRGNAME=stkchk
wcc -q -os %PRGNAME%.c
wlink system dos com option quiet,eliminate,map libfile ..\..\lib\com\wmincrtc.lib file %PRGNAME%

set PRGNAME=minimal
wcc -q -ox %PRGNAME%.c
wlink system dos com option quiet,eliminate,map libfile ..\..\lib\mincom\wmincrtc.lib file %PRGNAME%

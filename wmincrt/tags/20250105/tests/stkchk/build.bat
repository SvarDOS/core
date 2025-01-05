wasm -q ..\..\startup.asm
wcc -q -os -zl stkchk.c
wlink system dos com option quiet option map name stkchk file startup,stkchk

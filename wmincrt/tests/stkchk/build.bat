wasm -q -d1 -DSTACKSTAT ..\..\startup.asm
wcc -q -os -d2 stkchk.c
wlink system dos com debug all option quiet option map name stkchk file startup,stkchk
wasm -q -DNOSTACKCHECK ..\..\startup.asm
wcc -q -ox -s -zl minimal.c
wlink system dos com option quiet option map name hello file startup,minimal

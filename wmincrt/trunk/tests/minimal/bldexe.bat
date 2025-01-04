wasm -q -DNOSTACKCHECK -DEXE ..\..\startup.asm
wcc -q -ox -s -zl minimal.c
wlink system dos option quiet option map name hello file startup,minimal

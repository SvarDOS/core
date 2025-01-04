wasm -q -DNOARGV -DNOSTACKALLOC -DNOSTACKCHECK ..\..\startup.asm
wcc -q -ox -s -zl minimal.c
wlink system dos com option quiet option map name minimal file startup,minimal

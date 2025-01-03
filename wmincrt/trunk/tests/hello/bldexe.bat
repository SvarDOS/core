wasm -q -DNOSTACKCHECK -DEXE ..\..\startup.asm
wcc -q -os -s hello.c
wlink system dos option quiet option map name hello file startup,hello

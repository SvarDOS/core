wasm -q -DNOSTACKCHECK ..\..\startup.asm
wcc -q -os -s -zl hello.c
wlink system dos com option quiet option map name hello file startup,hello

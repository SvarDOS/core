wasm -q -d1 -DDEBUG ..\..\startup.asm
wcc -q -d2 -zl hello.c
wlink system dos com option quiet debug all option map name hello file startup,hello

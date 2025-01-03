wasm -q -d1 -DDEBUG -DEXE ..\..\startup.asm
wcc -q -d2 hello.c
wlink system dos option quiet debug all option map name hello file startup,hello

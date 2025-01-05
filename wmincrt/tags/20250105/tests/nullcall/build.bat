wasm -q -d1 -DDEBUG -DEXE ..\..\startup.asm
wcc -q -d2 -zl nullcall.c
wlink system dos option quiet debug all option map name nullcall file startup,nullcall

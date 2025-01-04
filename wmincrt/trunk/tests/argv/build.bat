wasm -q -d1 -DDEBUG -DEXE ..\..\startup.asm
wcc -q -d2 -zl argv.c
wlink system dos option quiet debug all option map name argv file startup,argv

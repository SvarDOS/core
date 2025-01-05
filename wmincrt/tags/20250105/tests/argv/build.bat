REM build smallest possible .COM
wasm -q -DNOSTACKALLOC -DNOSTACKCHECK ..\..\startup.asm
wcc -q -ox -zl argv.c
wlink system dos com option quiet option map name argv file startup,argv

REM build .EXE for debugging
wasm -q -d1 -DNOSTACKALLOC -DDEBUG -DEXE ..\..\startup.asm
wcc -q -d2 -zl argv.c
wlink system dos debug all option quiet option map name argv file startup,argv

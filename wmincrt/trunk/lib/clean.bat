@echo off
echo Cleaning default .COM library...
cd com
wmake -h -f Makefile clean
cd ..

cd exe
echo Cleaning default .EXE library...
wmake -h -f Makefile clean
cd ..

@echo off
echo Cleaning minimal .COM library...
cd mincom
wmake -h -f Makefile clean
cd ..

cd minexe
echo Cleaning minimal .EXE library...
wmake -h -f Makefile clean
cd ..

@echo off
echo Cleaning debug .COM library...
cd dbgcom
wmake -h -f Makefile clean
cd ..

cd dbgexe
echo Cleaning debug .EXE library...
wmake -h -f Makefile clean
cd ..

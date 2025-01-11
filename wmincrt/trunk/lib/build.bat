@echo off
echo Making default .COM library...
cd com
wmake -h -f Makefile
cd ..

cd exe
echo Making default .EXE library...
wmake -h -f Makefile
cd ..

@echo off
echo Making minimal .COM library...
cd mincom
wmake -h -f Makefile
cd ..

cd minexe
echo Making minimal .EXE library...
wmake -h -f Makefile
cd ..

@echo off
echo Making debug .COM library...
cd dbgcom
wmake -h -f Makefile
cd ..

cd dbgexe
echo Making debug .EXE library...
wmake -h -f Makefile
cd ..

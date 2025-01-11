# Watcom Makefile for WMINCRT

# This builds two libraries:
#   lib\wmincrtc.lib  (library for .COM files)
#   lib\wmincrte.lib  (library for .EXE files)

# The library can be customized by giving different command line arguments
# to WMAKE:
#   DEBUG=1		creates debug versions of the libraries
#   NOARGV=1		disables command line parsing and initializing
#			argc and argv
#   NOPRGNAME=1		disables getting executable name (argv[0]="")
#			only has effect if NOARGV is NOT defined

all:	.symbolic
	@echo Please build WMINCRT from one of the directories under LIB

clean: .symbolic
	rm -f tests/argv/*.com
	rm -f tests/argv/*.exe
	rm -f tests/argv/*.obj
	rm -f tests/argv/*.map
	rm -f tests/hello/*.com
	rm -f tests/hello/*.exe
	rm -f tests/hello/*.obj
	rm -f tests/hello/*.map
	rm -f tests/minimal/*.com
	rm -f tests/minimal/*.exe
	rm -f tests/minimal/*.obj
	rm -f tests/minimal/*.map
	rm -f tests/nullcall/*.exe
	rm -f tests/nullcall/*.obj
	rm -f tests/nullcall/*.map
	rm -f tests/stkchk/*.com
	rm -f tests/stkchk/*.exe
	rm -f tests/stkchk/*.obj
	rm -f tests/stkchk/*.map

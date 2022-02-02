#
# localcfg Makefile for Turbo C 2.01
#

all: localcfg.com

localcfg.com: localcfg.c country.c
	tcc -mt -lt -f- -O -Z -d -w localcfg.c country.c
	upx --8086 -9 localcfg.com

clean:
	del *.obj
	del *.com

buildpkg: localcfg.com
	mkdir appinfo
	mkdir source
	mkdir source\localcfg
	mkdir progs
	mkdir progs\localcfg
	copy localcfg.lsm appinfo
	copy localcfg.com progs\localcfg
	copy *.txt progs\localcfg
	copy *.c source\localcfg
	copy *.h source\localcfg
	copy *.txt source\localcfg
	copy makefile.* source\localcfg
	zip -9 -m -r localcfg.zip appinfo progs source

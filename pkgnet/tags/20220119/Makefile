#
# Makefile for DOS 16-bit (OpenWatcom 1.9)
#

CFLAGS = -j -ml -0 -bt=dos -wx -we -d0 -ox -dNOLFN -i=watt32\inc
LIB = watt32\lib\wattcpwl.lib

all: pkgnet.exe

release: pkgnet.exe .symbolic
	mkdir bin
	mkdir appinfo
	copy pkgnet.exe bin
	copy pkgnet.lsm appinfo
	zip -9 -k -r -m pkgnet.zip bin appinfo

pkgnet.exe: pkgnet.obj net.obj unchunk.obj lsm.obj helpers.obj trim.obj
	wcl -lr -k4096 $(LIB) pkgnet.obj net.obj unchunk.obj lsm.obj helpers.obj trim.obj -fe=pkgnet.exe

pkgnet.obj: pkgnet.c
	*wcc $(CFLAGS) pkgnet.c

trim.obj: ..\pkg\trim.c
	*wcc $(CFLAGS) ..\pkg\trim.c

lsm.obj: ..\pkg\lsm.c
	*wcc $(CFLAGS) ..\pkg\lsm.c

helpers.obj: ..\pkg\helpers.c
	*wcc $(CFLAGS) ..\pkg\helpers.c

net.obj: net.c
	*wcc $(CFLAGS) net.c

unchunk.obj: unchunk.c
	*wcc $(CFLAGS) unchunk.c

unchtest.obj: unchtest.c
	*wcc $(CFLAGS) unchtest.c

unchtest.exe: unchtest.obj unchunk.obj
	wcl -lr $(LIB) unchtest.obj unchunk.obj -fe=unchtest.exe

clean: .symbolic
	del *.obj
	del pkgnet.exe

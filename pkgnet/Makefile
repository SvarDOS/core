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
	zip -9 -r -m pkgnet.zip bin appinfo

pkgnet.exe: pkgnet.obj net.obj unchunk.obj
	wcl -lr -k8192 $(LIB) pkgnet.obj net.obj unchunk.obj -fe=pkgnet.exe

pkgnet.obj: pkgnet.c
	*wcc $(CFLAGS) pkgnet.c

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

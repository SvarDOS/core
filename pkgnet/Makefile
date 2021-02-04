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

pkgnet.exe: pkgnet.obj net.obj
	wcl -lr -k8192 $(LIB) *.obj -fe=pkgnet.exe

pkgnet.obj: pkgnet.c
	*wcc $(CFLAGS) pkgnet.c

net.obj: net.c
	*wcc $(CFLAGS) net.c

clean: .symbolic
	del *.obj
	del pkgnet.exe

#
# Makefile for DOS 16-bit (OpenWatcom 1.9)
#

CFLAGS = -j -ml -0 -bt=dos -wx -we -d0 -ox -dNOLFN -i=watt32\inc
LIB = watt32\lib\wattcpwl.lib svarlang.lib\svarlngl.lib

all: pkgnet.exe

release: pkgnet.exe .symbolic
	mkdir bin
	mkdir appinfo
	copy pkgnet.exe bin
	copy pkgnet.lng bin
	copy pkgnet.lsm appinfo
	if exist pkgnet.svp del pkgnet.svp
	zip -9krDX -m pkgnet.svp bin appinfo
	rmdir bin
	rmdir appinfo
	if exist pkgnet.zip del pkgnet.zip
	zip -9krDX pkgnet.zip Makefile *.c *.h nls_utf8 pkgnet.lsm watt32 svarlang.lib

pkgnet.exe: pkgnet.obj net.obj unchunk.obj lsm.obj helpers.obj trim.obj
	cd nls_utf8
	utf8tocp 850 de_utf8.txt > de.txt
	utf8tocp 850 br_utf8.txt > br.txt
	utf8tocp 437 en_utf8.txt > en.txt
	utf8tocp maz pl_utf8.txt > pl.txt
	..\svarlang.lib\tlumacz en br de pl > tlumacz.log
	del ??.txt
	move /y deflang.c ..\deflang.c
	move /y out.lng ..\pkgnet.lng
	cd ..
	*wcl -lr -k4096 $(LIB) pkgnet.obj deflang.c net.obj unchunk.obj lsm.obj helpers.obj trim.obj -fe=pkgnet.exe
	upx --8086 -9 pkgnet.exe

pkgnet.obj: pkgnet.c
	*wcc $(CFLAGS) pkgnet.c

trim.obj: ..\..\pkg\trunk\trim.c
	*wcc $(CFLAGS) ..\..\pkg\trunk\trim.c

lsm.obj: ..\..\pkg\trunk\lsm.c
	*wcc $(CFLAGS) ..\..\pkg\trunk\lsm.c

helpers.obj: ..\..\pkg\trunk\helpers.c
	*wcc $(CFLAGS) ..\..\pkg\trunk\helpers.c

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

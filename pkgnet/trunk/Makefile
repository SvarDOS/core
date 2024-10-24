#
# Makefile for DOS 16-bit (OpenWatcom 1.9)
#

!ifdef __UNIX__
SEP=/
MV = mv
!else
SEP=\
MV = move /Y
!endif

TLUMACZ = ..$(SEP)svarlang.lib$(SEP)tlumacz.exe

CFLAGS = -q -j -ml -0 -bt=dos -wx -we -d0 -ox -dNOLFN -i=watt32$(SEP)inc
LIB = watt32/lib/wattcpwl.lib svarlang.lib/svarlngl.lib

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

deflang.c:
	cd nls_utf8
	rm -f ??.txt
	utf8tocp 850 br_utf8.txt br.txt
	utf8tocp 850 de_utf8.txt de.txt
	utf8tocp 437 en_utf8.txt en.txt
	utf8tocp 850 fr_utf8.txt fr.txt
	utf8tocp maz pl_utf8.txt pl.txt
	utf8tocp 857 tr_utf8.txt tr.txt
	$(TLUMACZ) en br de fr pl tr
	rm -f ??.txt
	$(MV) deflang.c ..$(SEP)deflang.c
	$(MV) out.lng ..$(SEP)pkgnet.lng
	cd ..

pkgnet.exe: pkgnet.obj net.obj unchunk.obj lsm.obj helpers.obj trim.obj deflang.c
	*wcl -lr -k4096 $(LIB) pkgnet.obj deflang.c net.obj unchunk.obj lsm.obj helpers.obj trim.obj -fe=pkgnet.exe
	upx --8086 --best pkgnet.exe

pkgnet.obj: pkgnet.c
	*wcc $(CFLAGS) -fo=$*.obj $[@

trim.obj: ../../pkg/trunk/trim.c
	*wcc $(CFLAGS) -fo=$*.obj $[@

lsm.obj: ../../pkg/trunk/lsm.c
	*wcc $(CFLAGS) -fo=$*.obj $[@

helpers.obj: ../../pkg/trunk/helpers.c
	*wcc $(CFLAGS) -fo=$*.obj $[@

net.obj: net.c
	*wcc $(CFLAGS) -fo=$*.obj $[@

unchunk.obj: unchunk.c
	*wcc $(CFLAGS) -fo=$*.obj $[@

unchtest.obj: unchtest.c
	*wcc $(CFLAGS) -fo=$*.obj $[@

unchtest.exe: unchtest.obj unchunk.obj
	wcl -lr $(LIB) unchtest.obj unchunk.obj -fe=unchtest.exe

clean: .symbolic
	rm -f *.obj
	rm -f deflang.*
	rm -f pkgnet.exe
	rm -f pkgnet.lng

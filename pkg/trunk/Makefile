#
# pkg Makefile -- requires OpenWatcom (wmake)
# Copyright (C) 2021-2024 Mateusz Viste
#

!ifdef __UNIX__
SEP=/
MV = mv
CP = cp
!else
SEP=\
MV = move /Y
CP = copy
!endif

TLUMACZ = ..$(SEP)svarlang.lib$(SEP)tlumacz.exe

CFLAGS = -q -0 -mc -os -wx -we -d0 -i=zlib
LDFLAGS = -lr -fe=pkg.exe -fm=pkg.map
LIBS = zlib/zlib_c.lib svarlang.lib/svarlngc.lib

all: pkg.exe

release: pkg.exe .symbolic
	rm -f pkg.zip
	rm -f pkg.svp
	mkdir appinfo
	mkdir bin
	mkdir doc
	$(CP) pkg.lng bin
	$(CP) pkg.exe bin
	$(CP) pkg.lsm appinfo
	$(CP) pkg.txt doc
	zip -9moDkrX pkg.svp appinfo bin doc
	zip -9oDkrX pkg.zip nls_utf8 svarlang.lib zlib *.c *.h pkg.txt pkg.lsm makefile
	rmdir appinfo
	rmdir bin
	rmdir doc

pkg.exe: main.obj healthck.obj crc32.obj fileexst.obj helpers.obj inf.obj libunzip.obj loadconf.obj lsm.obj pkginst.obj pkgrem.obj trim.obj showinst.obj unzip.obj deflang.obj
	wcl $(LDFLAGS) $(LIBS) *.obj
	upx --8086 --best pkg.exe

deflang.obj: nls_utf8/pkg_??.txt
	cd nls_utf8
	rm -f ??.txt
	$(CP) pkg_en.txt en.txt
	utf8tocp 850 pkg_br.txt br.txt
	utf8tocp 850 pkg_de.txt de.txt
	utf8tocp 850 pkg_fr.txt fr.txt
	utf8tocp 850 pkg_dk.txt dk.txt
	utf8tocp 852 pkg_si.txt si.txt
	utf8tocp 857 pkg_tr.txt tr.txt
	utf8tocp maz pkg_pl.txt pl.txt
	$(TLUMACZ) en br de fr dk si tr pl > tlumacz.log
	$(MV) out.lng ..$(SEP)pkg.lng
	$(MV) deflang.c ..
	rm -f ??.txt
	cd ..
	wcc $(CFLAGS) -fo=deflang.obj deflang.c


main.obj: main.c
	wcc $(CFLAGS) -fo=$*.obj $[@

crc32.obj: crc32.c
	wcc $(CFLAGS) -fo=$*.obj $[@

fileexst.obj: fileexst.c
	wcc $(CFLAGS) -fo=$*.obj $[@

healthck.obj: healthck.c
	wcc $(CFLAGS) -fo=$*.obj $[@

helpers.obj: helpers.c
	wcc $(CFLAGS) -fo=$*.obj $[@

inf.obj: inf.c
	wcc $(CFLAGS) -fo=$*.obj $[@

libunzip.obj: libunzip.c
	wcc $(CFLAGS) -fo=$*.obj $[@

loadconf.obj: loadconf.c
	wcc $(CFLAGS) -fo=$*.obj $[@

lsm.obj: lsm.c
	wcc $(CFLAGS) -fo=$*.obj $[@

pkginst.obj: pkginst.c
	wcc $(CFLAGS) -fo=$*.obj $[@

pkgrem.obj: pkgrem.c
	wcc $(CFLAGS) -fo=$*.obj $[@

trim.obj: trim.c
	wcc $(CFLAGS) -fo=$*.obj $[@

showinst.obj: showinst.c
	wcc $(CFLAGS) -fo=$*.obj $[@

unzip.obj: unzip.c
	wcc $(CFLAGS) -fo=$*.obj $[@

clean: .symbolic
	rm -f *.obj
	rm -f pkg.exe
	rm -f pkg.svp
	rm -f pkg.zip
	rm -f pkg.lng
	rm -f pkg.map
	rm -f deflang.c
	rm -f nls_utf8/tlumacz.log


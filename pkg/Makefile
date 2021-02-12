#
# pkg Makefile -- requires OpenWatcom (wmake)
# Copyright (C) 2021 Mateusz Viste
#

CFLAGS = -0 -mc -os -wx -we -d0 -i=zlib
LDFLAGS = -lr -fe=pkg.exe
LIBS = zlib\zlib_c.lib

all: pkg.exe

release: pkg.exe .symbolic
	mkdir appinfo
	mkdir bin
	mkdir nls
	copy nls_utf8\pkg_en.txt nls\pkg.en
	utf8tocp 850 nls_utf8\pkg_de.txt > nls\pkg.de
	utf8tocp 850 nls_utf8\pkg_fr.txt > nls\pkg.fr
	utf8tocp 850 nls_utf8\pkg_dk.txt > nls\pkg.dk
	utf8tocp 852 nls_utf8\pkg_si.txt > nls\pkg.si
	utf8tocp 857 nls_utf8\pkg_tr.txt > nls\pkg.tr
	utf8tocp maz nls_utf8\pkg_pl.txt > nls\pkg.pl
	upx --8086 -9 pkg.exe -o bin\pkg.exe
	copy pkg.lsm appinfo
	zip -9 -m -o -r pkg.zip appinfo bin nls

pkg.exe: kitten.obj main.obj crc32.obj fileexst.obj helpers.obj inf.obj kprintf.obj libunzip.obj loadconf.obj lsm.obj pkginst.obj pkgrem.obj trim.obj showinst.obj unzip.obj
	wcl $(LDFLAGS) $(LIBS) *.obj

kitten.obj: kitten\kitten.c
	wcc $(CFLAGS) kitten\kitten.c

main.obj: main.c
	wcc $(CFLAGS) main.c

crc32.obj: crc32.c
	wcc $(CFLAGS) crc32.c

fileexst.obj: fileexst.c
	wcc $(CFLAGS) fileexst.c

helpers.obj: helpers.c
	wcc $(CFLAGS) helpers.c

inf.obj: inf.c
	wcc $(CFLAGS) inf.c

kprintf.obj: kprintf.c
	wcc $(CFLAGS) kprintf.c

libunzip.obj: libunzip.c
	wcc $(CFLAGS) libunzip.c

loadconf.obj: loadconf.c
	wcc $(CFLAGS) loadconf.c

lsm.obj: lsm.c
	wcc $(CFLAGS) lsm.c

pkginst.obj: pkginst.c
	wcc $(CFLAGS) pkginst.c

pkgrem.obj: pkgrem.c
	wcc $(CFLAGS) pkgrem.c

trim.obj: trim.c
	wcc $(CFLAGS) trim.c

showinst.obj: showinst.c
	wcc $(CFLAGS) showinst.c

unzip.obj: unzip.c
	wcc $(CFLAGS) unzip.c

clean: .symbolic
	del *.obj
	del pkg.exe

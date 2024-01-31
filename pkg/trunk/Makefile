#
# pkg Makefile -- requires OpenWatcom (wmake)
# Copyright (C) 2021-2024 Mateusz Viste
#

CFLAGS = -0 -mc -os -wx -we -d0 -i=zlib
LDFLAGS = -lr -fe=pkg.exe -fm=pkg.map
LIBS = zlib\zlib_c.lib svarlang.lib\svarlngc.lib

all: pkg.exe

release: pkg.exe .symbolic
	IF EXIST pkg.zip DEL pkg.zip
	IF EXIST pkg.svp DEL pkg.svp
	mkdir appinfo
	mkdir bin
	copy pkg.lng bin
	upx --8086 -9 pkg.exe -o bin\pkg.exe
	copy pkg.lsm appinfo
	zip -9moDkrX pkg.svp appinfo bin
	zip -9oDkrX pkg.zip nls_utf8 svarlang.lib zlib *.c *.h pkg.lsm makefile
	rmdir appinfo
	rmdir bin

pkg.exe: main.obj crc32.obj fileexst.obj helpers.obj inf.obj kprintf.obj libunzip.obj loadconf.obj lsm.obj pkginst.obj pkgrem.obj trim.obj showinst.obj unzip.obj deflang.obj
	wcl $(LDFLAGS) $(LIBS) *.obj

deflang.obj: nls_utf8\pkg_en.txt
	cd nls_utf8
	copy pkg_en.txt en.txt
	utf8tocp 850 pkg_br.txt > br.txt
	utf8tocp 850 pkg_de.txt > de.txt
	utf8tocp 850 pkg_fr.txt > fr.txt
	utf8tocp 850 pkg_dk.txt > dk.txt
	utf8tocp 852 pkg_si.txt > si.txt
	utf8tocp 857 pkg_tr.txt > tr.txt
	utf8tocp maz pkg_pl.txt > pl.txt
	..\svarlang.lib\tlumacz en br de fr dk si tr pl > tlumacz.log
	move /y out.lng ..\pkg.lng
	move /y deflang.c ..
	del ??.txt
	cd ..
	wcc $(CFLAGS) deflang.c

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

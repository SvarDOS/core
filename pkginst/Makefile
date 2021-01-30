#
# pkginst Makefile -- requires OpenWatcom (wmake)
# Copyright (C) 2021 Mateusz Viste
#

CFLAGS = -0 -ml -os -wx -we -d0 -i=zlib
LDFLAGS = -ml -lr -fe=pkginst.exe
LIBS = zlib\zlib_l.lib

all: pkginst.exe

pkginst.exe: main.obj crc32.obj fileexst.obj getdelim.obj helpers.obj inf.obj kprintf.obj libunzip.obj loadconf.obj lsm.obj parsecmd.obj pkginst.obj pkgrem.obj rtrim.obj showinst.obj
	wcl $(LDFLAGS) $(LIBS) *.obj

main.obj: main.c
	wcc $(CFLAGS) main.c

crc32.obj: crc32.c
	wcc $(CFLAGS) crc32.c

fileexst.obj: fileexst.c
	wcc $(CFLAGS) fileexst.c

getdelim.obj: getdelim.c
	wcc $(CFLAGS) getdelim.c

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

parsecmd.obj: parsecmd.c
	wcc $(CFLAGS) parsecmd.c

pkginst.obj: pkginst.c
	wcc $(CFLAGS) pkginst.c

pkgrem.obj: pkgrem.c
	wcc $(CFLAGS) pkgrem.c

rtrim.obj: rtrim.c
	wcc $(CFLAGS) rtrim.c

showinst.obj: showinst.c
	wcc $(CFLAGS) showinst.c

clean: .symbolic
	del *.obj
	del pkginst.exe

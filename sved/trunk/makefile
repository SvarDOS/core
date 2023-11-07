#
# sved's makefile -- requires Open Watcom 1.9 (wmake)
# Copyright (C) 2023 Mateusz Viste
#
# usage:
#   wmake            builds the sved.com binary
#   wmake release    builds distribuable packages (svp, zip, sources...)

CC = wcc
CFLAGS = -0 -ms -os -wx -we -d0 -bt=dos -I=mdr\inc
CFLAGS += -s
LDFLAGS = -lr -mt -fm=sved.map -fe=sved.com
LDLIBS = svarlang\svarlngs.lib mdr\mdrs2024.lib

PVER = 2023.5

all: sved.com

sved.com: sved.obj deflang.obj
	wcl $(LDLIBS) sved.obj deflang.obj $(LDFLAGS)
	upx -9 --8086 sved.com

.c.obj: .autodepend
	$(CC) $(CFLAGS) $*.C

# builds a SvarDOS package
sved.svp: sved.com
	IF EXIST SVED.SVP DEL SVED.SVP
	mkdir bin
	mkdir appinfo
	mkdir doc
	mkdir doc\sved
	copy sved.com bin
	copy sved.lng bin
	copy sved.txt doc\sved
	echo version: $(PVER) > appinfo\sved.lsm
	echo description: SvarDOS text editor >> appinfo\sved.lsm
	zip -9rkDX -m sved.svp bin appinfo doc
	rmdir doc\sved
	rmdir doc
	rmdir bin
	rmdir appinfo

# "normal" DOS distribution, ie. bunch of binaries in a zip file
sved.zip: sved.com
	IF EXIST SVED.ZIP DEL SVED.ZIP
	zip -9rkDX sved.zip sved.com sved.lng sved.txt

# source package (no binaries)
sved_src.zip:
	IF EXIST SVED_SRC.ZIP DEL SVED_SRC.ZIP
	zip -9rkDX sved_src.zip *.txt *.c *.h *.lng makefile mdr nls svarlang

release: sved.svp sved.zip sved_src.zip .symbolic

clean: .symbolic
	del *.obj
	del sved.com

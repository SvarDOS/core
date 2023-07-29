#
# sved Makefile -- requires OpenWatcom (wmake)
# Copyright (C) 2023 Mateusz Viste
#

CC = wcc
CFLAGS = -0 -ms -os -wx -we -d0 -I=mdr\inc
CFLAGS += -s
LDFLAGS = -lr -mt -fm=edit.map -fe=edit.com
LDLIBS = svarlang\svarlngs.lib mdr\mdrs2023.lib

all: edit.com

edit.com: sved.obj deflang.obj
	wcl $(LDLIBS) sved.obj deflang.obj $(LDFLAGS)
	upx -9 --8086 edit.com

.c.obj: .autodepend
	$(CC) $(CFLAGS) $*.C

pkg: edit.com .symbolic
	mkdir bin
	mkdir appinfo
	mkdir doc
	mkdir doc\sved
	copy edit.com bin
	copy edit.lng bin
	copy sved.txt doc\sved
	echo version: 2023.0 > appinfo\sved.lsm
	echo description: SvarDOS text editor >> appinfo\sved.lsm
	zip -9rkDX -m sved.zip bin appinfo doc
	rmdir doc\sved
	rmdir doc
	rmdir bin
	rmdir appinfo

clean: .symbolic
	del *.obj
	del edit.com

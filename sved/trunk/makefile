#
# sved Makefile -- requires OpenWatcom (wmake)
# Copyright (C) 2023 Mateusz Viste
#

CC = wcc
CFLAGS = -0 -ms -os -wx -we -d0 -I=mdr\inc
CFLAGS += -s
LDFLAGS = -lr -mt -fm=sved.map -fe=sved.com
LDLIBS = svarlang\svarlngs.lib mdr\mdrs2023.lib

all: sved.com

sved.com: sved.obj deflang.obj
	wcl $(LDLIBS) sved.obj deflang.obj $(LDFLAGS)

.c.obj: .autodepend
	$(CC) $(CFLAGS) $*.C

clean: .symbolic
	del *.obj
	del sved.com

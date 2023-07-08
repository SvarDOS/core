#
# sved Makefile -- requires OpenWatcom (wmake)
# Copyright (C) 2023 Mateusz Viste
#

CC = wcc
CFLAGS = -0 -mc -os -wx -we -d0 -I=mdr\inc
LDFLAGS = -lr -fm=sved.map -fe=sved.exe
LDLIBS = svarlang.lib\20230630\svarlngc.lib mdr\mdrc2023.lib

all: sved.exe

sved.exe: sved.obj
	wcl sved.obj $(LDLIBS) $(LDFLAGS)

.c.obj: .autodepend
	$(CC) $(CFLAGS) $*.C

clean: .symbolic
	del *.obj
	del sved.exe

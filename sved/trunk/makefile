#
# sved Makefile -- requires OpenWatcom (wmake)
# Copyright (C) 2023 Mateusz Viste
#

CC = wcc
CFLAGS = -0 -mc -os -wx -we -d0 -I=mdr\inc
CFLAGS += -s
LDFLAGS = -lr -fm=sved.map -fe=sved.exe
LDLIBS = svarlang\svarlngc.lib mdr\mdrc2023.lib

all: sved.exe

sved.exe: sved.obj deflang.obj
	wcl $(LDLIBS) sved.obj deflang.obj $(LDFLAGS)

.c.obj: .autodepend
	$(CC) $(CFLAGS) $*.C

clean: .symbolic
	del *.obj
	del sved.exe

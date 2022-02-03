#
# make instructions to build svarlang and tlumacz.exe with OpenWatcom
# Copyright (C) 2021-2022 Mateusz Viste
#

all: svarlngs.lib svarlngc.lib svarlngm.lib svarlngl.lib tlumacz.exe

CFLAGS=-0 -wx -we -ox

svarlngs.lib: autoload.c svarlang.c
	wcc $(CFLAGS) -ms autoload.c
	wcc $(CFLAGS) -ms svarlang.c
	wlib svarlngs.lib +autoload.obj +svarlang.obj

svarlngc.lib: autoload.c svarlang.c
	wcc $(CFLAGS) -mc autoload.c
	wcc $(CFLAGS) -mc svarlang.c
	wlib svarlngc.lib +autoload.obj +svarlang.obj

svarlngm.lib: autoload.c svarlang.c
	wcc $(CFLAGS) -mm autoload.c
	wcc $(CFLAGS) -mm svarlang.c
	wlib svarlngm.lib +autoload.obj +svarlang.obj

svarlngl.lib: autoload.c svarlang.c
	wcc $(CFLAGS) -ml autoload.c
	wcc $(CFLAGS) -ml svarlang.c
	wlib svarlngl.lib +autoload.obj +svarlang.obj


tlumacz.exe: tlumacz.c
	wcl -0 -y -cc -wx -mc -lr -we -ox tlumacz.c
	del *.obj

clean: .symbolic
	del *.exe
	del *.obj

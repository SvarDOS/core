#
# Makefile for OpenWatcom (wmake)
#

CC=wcl
CFLAGS=-ms -0 -ox -d0 -wx -we

all: tree.exe

tree.exe: tree.c deflang.c stack.c
	$(CC) $(CFLAGS) tree.c deflang.c stack.c svarlang\svarlngs.lib

deflang.c: nls\*.txt
	if exist del deflang.c
	if exist tree.lng del tree.lng
	cd nls
	..\svarlang\tlumacz en es
	copy out.lng ..\tree.lng
	copy deflang.c ..
	del out.lng
	del deflang.c
	cd ..

clean: .symbolic
	del tree.exe
	del tree.lng
	del deflang.c
	del *.obj

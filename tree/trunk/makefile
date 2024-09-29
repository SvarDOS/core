#
# Makefile for OpenWatcom (wmake)
# [DOS]
#

CC=wcl
CFLAGS=-mt -s -0 -ox -d0 -wx -we -fm=tree.map

all: tree.com

tree.com: tree.c deflang.c stack.c
	$(CC) $(CFLAGS) tree.c deflang.c stack.c svarlang\svarlngs.lib

deflang.c: nls\*.txt
	if exist del deflang.c
	if exist tree.lng del tree.lng
	cd nls
	utf8tocp 437 en-utf8.txt en.txt
	utf8tocp 850 es-utf8.txt es.txt
	utf8tocp 850 fi-utf8.txt fi.txt
	utf8tocp 775 lv-utf8.txt lv.txt
	utf8tocp 850 pt-utf8.txt pt.txt
	utf8tocp 866 ru-utf8.txt ru.txt
	utf8tocp 857 tr-utf8.txt tr.txt
	..\svarlang\tlumacz /excref en es fi lv pt ru tr
	copy out.lng ..\tree.lng
	copy deflang.c ..
	del out.lng
	del deflang.c
	del ??.txt
	cd ..

clean: .symbolic
	if exist tree.com del tree.com
	if exist tree.lng del tree.lng
	if exist deflang.c del deflang.c
	del *.obj

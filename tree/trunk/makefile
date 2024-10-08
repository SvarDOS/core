#
# Makefile for OpenWatcom (wmake)
#

CC=wcl
CFLAGS=-mt -s -0 -ox -d0 -wx -we -fm=tree.map
!ifdef __UNIX__
CP=cp
SEP=/
!else
CP=copy
SEP=\
!endif

all: tree.com

tree.com: tree.c deflang.c stack.c
	$(CC) $(CFLAGS) tree.c deflang.c stack.c svarlang/svarlngs.lib

release: tree.com .symbolic
	mkdir appinfo
	mkdir bin
	mkdir doc
	copy tree.lsm appinfo
	copy tree.com bin
	copy tree.lng bin
	copy tree.txt doc
	rm -f tree.svp
	zip -r9kDX -m tree.svp appinfo bin doc
	rmdir appinfo
	rmdir bin
	rmdir doc
	rm -f tree-src.zip
	zip -r9kDX tree-src.zip makefile *.c *.h *.lsm *.txt nls svarlang

deflang.c: nls/*.txt
	rm -f deflang.c
	rm -f tree.lng
	cd nls
	utf8tocp 437 en-utf8.txt en.txt
	utf8tocp 850 de-utf8.txt de.txt
	utf8tocp 850 es-utf8.txt es.txt
	utf8tocp 850 fi-utf8.txt fi.txt
	utf8tocp 775 lv-utf8.txt lv.txt
	utf8tocp 850 pt-utf8.txt pt.txt
	utf8tocp 866 ru-utf8.txt ru.txt
	utf8tocp 857 tr-utf8.txt tr.txt
	..$(SEP)svarlang$(SEP)tlumacz /excref en de es fi lv pt ru tr > tlumacz.log
	$(CP) out.lng ..$(SEP)tree.lng
	$(CP) deflang.c ..
	rm -f out.lng
	rm -f deflang.c
	rm -f ??.txt
	cd ..

clean: .symbolic
	rm -f nls/??.txt
	rm -f tree.com
	rm -f tree.lng
	rm -f tree.map
	rm -f deflang.c
	rm -f *.obj
	rm -f *.o


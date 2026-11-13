#
# SVARDISK makefile -- requires Open Watcom 1.9 (wmake)
# Copyright (C) 2026 Mateusz Viste
#
#   wmake            builds svardisk.exe
#   wmake host       Linux program that checks FAT image files
#   wmake test       builds the Linux program and runs the image tests
#   wmake release    builds svardisk.svp, svardisk.zip and svardisk_src.zip
#
# On Linux, gcc compiles tlumacz and the host program. utf8tocp must be
# on PATH. A DOS build uses svarlang\tlumacz.exe.

CC = wcc
CFLAGS = -q -0 -bt=dos -ms -zl -os -s -wx -we -i=wmincrt -i=svarlang
HOSTCFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -DSV_HOST -I. -Isvarlang

PVER = 2026.0

!ifdef __UNIX__
SEP = /
CP = cp
RM = rm -f
TLUMACZ = ..$(SEP)tlumacz
!else
SEP = \
CP = copy
RM = del
TLUMACZ = ..$(SEP)svarlang$(SEP)tlumacz.exe
!endif

all: svardisk.exe .symbolic

!ifdef __UNIX__
tlumacz: svarlang$(SEP)tlumacz.c svarlang$(SEP)mvlib$(SEP)mvpack.c
	gcc -O2 -I svarlang -I svarlang$(SEP)mvlib -o tlumacz svarlang$(SEP)tlumacz.c svarlang$(SEP)mvlib$(SEP)mvpack.c

host: svardisk .symbolic

test: svardisk .symbolic
	python3 tests$(SEP)test_svardisk.py

svardisk: deflang.c svardisk.c out.c diskio.c bitmap.c bpb.c fat.c walk.c svardisk.h
	gcc $(HOSTCFLAGS) -c svardisk.c -o svardisk.o
	gcc $(HOSTCFLAGS) -c out.c -o out.o
	gcc $(HOSTCFLAGS) -c diskio.c -o diskio.o
	gcc $(HOSTCFLAGS) -c bitmap.c -o bitmap.o
	gcc $(HOSTCFLAGS) -c bpb.c -o bpb.o
	gcc $(HOSTCFLAGS) -c fat.c -o fat.o
	gcc $(HOSTCFLAGS) -c walk.c -o walk.o
	gcc -O2 -c deflang.c -o deflang.o
	gcc -O2 -DWITHSTDIO -Isvarlang -c svarlang$(SEP)svarlang.c -o svarlang_h.o
	gcc -O2 -Isvarlang -c svarlang$(SEP)auto_exe.c -o auto_exe_h.o
	gcc -o svardisk svardisk.o out.o diskio.o bitmap.o bpb.o fat.o walk.o deflang.o svarlang_h.o auto_exe_h.o
!endif

svardisk.exe: startup.obj u4m.obj u4d.obj svardisk.obj out.obj diskio.obj bitmap.obj bpb.obj fat.obj walk.obj deflang.obj svarlang.obj auto_exe.obj svardisk.lnk
	wlink @svardisk.lnk
	upx -9 --8086 svardisk.exe

startup.obj: wmincrt$(SEP)startup.asm
	wasm -q -DEXE -DPRGNAME -DSTACKSIZE=8192 -DNOSTACKCHECK -i=wmincrt -fo=startup.obj wmincrt$(SEP)startup.asm

u4m.obj: wmincrt$(SEP)u4m.asm
	wasm -q -DEXE -i=wmincrt -fo=u4m.obj wmincrt$(SEP)u4m.asm

u4d.obj: u4d.asm
	wasm -q -DEXE -i=wmincrt -fo=u4d.obj u4d.asm

svardisk.obj: svardisk.c svardisk.h
	$(CC) $(CFLAGS) -fo=svardisk.obj svardisk.c

out.obj: out.c svardisk.h
	$(CC) $(CFLAGS) -fo=out.obj out.c

diskio.obj: diskio.c svardisk.h
	$(CC) $(CFLAGS) -fo=diskio.obj diskio.c

bitmap.obj: bitmap.c svardisk.h
	$(CC) $(CFLAGS) -fo=bitmap.obj bitmap.c

bpb.obj: bpb.c svardisk.h
	$(CC) $(CFLAGS) -fo=bpb.obj bpb.c

fat.obj: fat.c svardisk.h
	$(CC) $(CFLAGS) -fo=fat.obj fat.c

walk.obj: walk.c svardisk.h
	$(CC) $(CFLAGS) -fo=walk.obj walk.c

deflang.obj: deflang.c
	$(CC) $(CFLAGS) -fo=deflang.obj deflang.c

svarlang.obj: svarlang$(SEP)svarlang.c svarlang$(SEP)svarlang.h
	$(CC) $(CFLAGS) -fo=svarlang.obj svarlang$(SEP)svarlang.c

auto_exe.obj: svarlang$(SEP)auto_exe.c svarlang$(SEP)svarlang.h
	$(CC) $(CFLAGS) -fo=auto_exe.obj svarlang$(SEP)auto_exe.c

!ifdef __UNIX__
deflang.c: tlumacz nls$(SEP)en_utf8.txt .symbolic
!else
deflang.c: nls$(SEP)en_utf8.txt .symbolic
!endif
	cd nls
	$(RM) en.txt
	utf8tocp 437 en_utf8.txt en.txt
	$(TLUMACZ) /nomarg /excref en > tlumacz.log
!ifdef __UNIX__
	$(CP) out.lng ..$(SEP)svardisk.lng
	$(CP) deflang.c ..
!else
	IF EXIST ..\svardisk.lng DEL ..\svardisk.lng
	$(CP) out.lng ..\svardisk.lng
	IF EXIST ..\deflang.c DEL ..\deflang.c
	$(CP) deflang.c ..
!endif
	$(RM) out.lng
	$(RM) deflang.c
	$(RM) en.txt
	$(RM) tlumacz.log
	cd ..

svardisk.svp: svardisk.exe svardisk.lng svardisk.txt .symbolic
	$(RM) svardisk.svp
!ifdef __UNIX__
	rm -rf bin appinfo doc
!endif
	mkdir bin
	mkdir appinfo
	mkdir doc
	$(CP) svardisk.exe bin
	$(CP) svardisk.lng bin
	$(CP) svardisk.txt doc
!ifdef __UNIX__
	printf 'version: $(PVER)\r\ndescription: checks and repairs FAT12, FAT16 and FAT32 disks\r\nhwreq: 8086\r\n' > appinfo/svardisk.lsm
!else
	echo version: $(PVER)> appinfo\svardisk.lsm
	echo description: checks and repairs FAT12, FAT16 and FAT32 disks>> appinfo\svardisk.lsm
	echo hwreq: 8086>> appinfo\svardisk.lsm
!endif
	zip -9rkDX -m svardisk.svp bin appinfo doc
	rmdir bin
	rmdir appinfo
	rmdir doc

svardisk.zip: svardisk.exe svardisk.lng svardisk.txt .symbolic
	$(RM) svardisk.zip
	zip -9rkDX svardisk.zip svardisk.exe svardisk.lng svardisk.txt

svardisk_src.zip: .symbolic
	$(RM) svardisk_src.zip
	$(RM) nls$(SEP)en.txt
	$(RM) nls$(SEP)tlumacz.log
	$(RM) deflang.c
!ifdef __UNIX__
	rm -rf tests$(SEP)__pycache__
!endif
	zip -9rDX svardisk_src.zip makefile *.c *.h *.asm *.txt *.lnk nls tests

release: svardisk.svp svardisk.zip svardisk_src.zip .symbolic

clean: .symbolic
	$(RM) *.obj
	$(RM) *.o
	$(RM) svardisk.exe
	$(RM) svardisk.lng
	$(RM) svardisk.map
	$(RM) deflang.c
	$(RM) tlumacz
	$(RM) nls$(SEP)en.txt
	$(RM) nls$(SEP)tlumacz.log
	$(RM) svardisk.svp
	$(RM) svardisk.zip
	$(RM) svardisk_src.zip
!ifdef __UNIX__
	$(RM) svardisk
!endif

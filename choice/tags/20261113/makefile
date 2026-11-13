#
# CHOICE makefile -- requires Open Watcom 1.9 (wmake)
# Copyright (C) 2026 Mateusz Viste
#
#   wmake            builds choice.com
#   wmake release    builds choice.svp, choice.zip and choice_src.zip
#
# On Linux, gcc is used once to compile tlumacz. utf8tocp must be on PATH.
# A DOS build runs svarlang\tlumacz.exe. Both compress the LNG.
# choice.lnk keeps the wlink line inside the DOS 127-character limit.

CC = wcc
CFLAGS = -q -0 -bt=dos -ms -zl -os -s -wx -we -i=wmincrt -i=svarlang

PVER = 20261113

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

TLFLAGS = /nomarg /excref

LANGS = en de fr pl es pt br nl ru tr

all: choice.com .symbolic

!ifdef __UNIX__
tlumacz: svarlang$(SEP)tlumacz.c svarlang$(SEP)mvlib$(SEP)mvpack.c
	gcc -O2 -I svarlang -I svarlang$(SEP)mvlib -o tlumacz svarlang$(SEP)tlumacz.c svarlang$(SEP)mvlib$(SEP)mvpack.c
!endif

choice.com: startup.obj choice.obj deflang.obj choice.lnk
	wlink @choice.lnk
	upx -9 --8086 choice.com

startup.obj: wmincrt$(SEP)startup.asm
	wasm -q -DNOARGV -DNOSTACKCHECK -i=wmincrt -fo=startup.obj wmincrt$(SEP)startup.asm

choice.obj: choice.c
	$(CC) $(CFLAGS) -fo=choice.obj choice.c

deflang.obj: deflang.c
	$(CC) $(CFLAGS) -fo=deflang.obj deflang.c

!ifdef __UNIX__
deflang.c: tlumacz nls$(SEP)en_utf8.txt .symbolic
!else
deflang.c: nls$(SEP)en_utf8.txt .symbolic
!endif
	cd nls
	$(RM) ??.txt
	utf8tocp 437 en_utf8.txt en.txt
	utf8tocp 850 de_utf8.txt de.txt
	utf8tocp 850 fr_utf8.txt fr.txt
	utf8tocp maz pl_utf8.txt pl.txt
	utf8tocp 850 es_utf8.txt es.txt
	utf8tocp 850 pt_utf8.txt pt.txt
	utf8tocp 850 br_utf8.txt br.txt
	utf8tocp 850 nl_utf8.txt nl.txt
	utf8tocp 866 ru_utf8.txt ru.txt
	utf8tocp 857 tr_utf8.txt tr.txt
	$(TLUMACZ) $(TLFLAGS) $(LANGS) > tlumacz.log
	-$(RM) ..$(SEP)choice.lng
	$(CP) out.lng ..$(SEP)choice.lng
	-$(RM) ..$(SEP)deflang.c
	$(CP) deflang.c ..
	$(RM) out.lng
	$(RM) deflang.c
	$(RM) ??.txt
	$(RM) tlumacz.log
	cd ..

choice.svp: choice.com .symbolic
	$(RM) choice.svp
!ifdef __UNIX__
	rm -rf bin appinfo doc
!endif
	mkdir bin
	mkdir appinfo
	mkdir doc
	$(CP) choice.com bin
	$(CP) choice.lng bin
	$(CP) choice.txt doc
!ifdef __UNIX__
	printf 'version: $(PVER)\r\ndescription: presents a choice to the user and waits for a key\r\nhwreq: 8086\r\n' > appinfo/choice.lsm
!else
	echo version: $(PVER)> appinfo\choice.lsm
	echo description: presents a choice to the user and waits for a key>> appinfo\choice.lsm
	echo hwreq: 8086>> appinfo\choice.lsm
!endif
	zip -9rkDX -m choice.svp bin appinfo doc
	rmdir bin
	rmdir appinfo
	rmdir doc

choice.zip: choice.com .symbolic
	$(RM) choice.zip
	zip -9rkDX choice.zip choice.com choice.lng choice.txt

choice_src.zip: .symbolic
	$(RM) choice_src.zip
	$(RM) nls$(SEP)tlumacz.log
	zip -9rkDX choice_src.zip makefile choice.lnk *.c *.txt nls tests svarlang wmincrt

release: choice.svp choice.zip choice_src.zip .symbolic

clean: .symbolic
	$(RM) *.obj
	$(RM) choice.com
	$(RM) choice.lng
	$(RM) choice.map
	$(RM) deflang.c
	$(RM) tlumacz
	$(RM) nls$(SEP)??.txt
	$(RM) nls$(SEP)tlumacz.log
	$(RM) choice.svp
	$(RM) choice.zip
	$(RM) choice_src.zip

#
# This is a makefile to build the SvarDOS install program
#
# You can use following targets:
#
#  wmake           - compiles the program
#  wmake clean     - cleans up all non-source files
#

all: install.com

install.com: keylay.h install.c input.c video.c deflang.c
	wcl -0 -y -cc -wx -mt -lr -we -d0 -ox install.c input.c video.c deflang.c svarlang.lib\svarlngs.lib
	upx --8086 -9 install.com

deflang.c:
	cd nls
	utf8tocp 437 en_utf8.txt > en.txt
	utf8tocp 858 de_utf8.txt > de.txt
	utf8tocp 858 fr_utf8.txt > fr.txt
	utf8tocp 858 it_utf8.txt > it.txt
	utf8tocp maz pl_utf8.txt > pl.txt
	utf8tocp 866 ru_utf8.txt > ru.txt
	utf8tocp 852 si_utf8.txt > si.txt
	utf8tocp 858 sv_utf8.txt > sv.txt
	utf8tocp 857 tr_utf8.txt > tr.txt
	..\svarlang.lib\tlumacz en de fr it pl ru si sv tr
	if exist ..\deflang.c del ..\deflang.c
	if exist ..\install.lng del ..\install.lng
	move deflang.c ..
	move out.lng ..\install.lng
	del ??.txt
	cd ..

locales.exe: locales.c
	wcl -0 -y -cc -wx -ml -lr -we -d0 -ox locales.c

keylay.h: locales.exe
	locales.exe

clean: .SYMBOLIC
	del install.com
	del locales.exe
	del *.obj
	del deflang.c
	del install.lng

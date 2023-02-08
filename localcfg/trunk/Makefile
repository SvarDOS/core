#
# localcfg Makefile for OpenWatcom
#

SVARLANGVER=20220314

all: localcfg.com

localcfg.com: localcfg.c country.c deflang.c
	*wcl -0 -y -cc -wx -mt -lr -zp1 -we -d0 -ox -I=svarlang.lib\tags\$SVARLANGVER localcfg.c country.c deflang.c svarlang.lib/tags/$SVARLANGVER/svarlngs.lib
	del *.obj
	upx --8086 -9 localcfg.com

deflang.c: nls_lang\*.txt
	cd nls_lang
	utf8tocp 850 br_utf8.txt > br.txt
	utf8tocp 850 de_utf8.txt > de.txt
	utf8tocp maz pl_utf8.txt > pl.txt
	..\svarlang.lib\tags\$SVARLANGVER\tlumacz en br de pl > tlumacz.log
	move out.lng ..\localcfg.lng
	move deflang.c ..\
	cd ..

clean: .SYMBOLIC
	del *.obj
	del *.com
	del deflang.c
	del localcfg.lng

release: localcfg.com .SYMBOLIC
	mkdir appinfo
	mkdir bin
	mkdir doc
	mkdir doc\localcfg
	mkdir nls
	copy localcfg.lsm appinfo
	copy localcfg.com bin
	copy localcfg.lng nls
	copy *.txt doc\localcfg
	if exist localcfg.zip del localcfg.zip
	zip -9rkDX -m localcfg.svp appinfo bin doc nls
	zip -9rkDX localcfg.zip *.txt *.c *.h makefile svarlang.lib\tags\$SVARLANGVER nls_lang
	rmdir appinfo
	rmdir bin
	rmdir doc\localcfg
	rmdir doc
	rmdir nls

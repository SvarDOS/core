#
# localcfg Makefile for OpenWatcom
#

all: localcfg.com

localcfg.com: localcfg.c country.c deflang.c
	wcl -0 -y -cc -wx -mt -lr -zp1 -we -d0 -ox localcfg.c country.c deflang.c svarlang.lib/svarlngs.lib
	del *.obj
	upx --8086 -9 localcfg.com

deflang.c: nls_lang\*.txt
	cd nls_lang
	utf8tocp 850 de_utf8.txt > de.txt
	utf8tocp maz pl_utf8.txt > pl.txt
	..\svarlang.lib\tlumacz en de pl
	move out.lng ..\localcfg.lng
	move deflang.c ..\
	cd ..

clean: .SYMBOLIC
	del *.obj
	del *.com
	del deflang.c
	del localcfg.lng

buildpkg: localcfg.com .SYMBOLIC
	mkdir appinfo
	mkdir source
	mkdir source\localcfg
	mkdir source\localcfg\nls_lang
	mkdir source\localcfg\svarlang.lib
	mkdir bin
	mkdir doc
	mkdir doc\localcfg
	mkdir nls
	copy localcfg.lsm appinfo
	copy localcfg.com bin
	copy localcfg.lng nls
	copy svarlang.lib\*.* source\localcfg\svarlang.lib
	del source\localcfg\svarlang.lib\*.lib
	del source\localcfg\svarlang.lib\*.exe
	copy *.txt doc\localcfg
	copy *.c source\localcfg
	copy *.h source\localcfg
	copy *.txt source\localcfg
	copy makefile source\localcfg
	copy nls_lang\*.* source\localcfg\nls_lang
	if exist localcfg.zip del localcfg.zip
	zip -9rkDX -m localcfg.zip appinfo bin doc nls source
	rmdir appinfo
	rmdir source\localcfg\svarlang.lib
	rmdir source\localcfg\nls_lang
	rmdir source\localcfg
	rmdir source
	rmdir bin
	rmdir doc\localcfg
	rmdir doc
	rmdir nls

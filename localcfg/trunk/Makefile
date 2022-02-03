#
# localcfg Makefile for OpenWatcom
#

all: localcfg.com

localcfg.com: localcfg.c country.c deflang.c
	wcl -0 -y -cc -wx -mt -lr -zp1 -we -d0 -ox localcfg.c country.c deflang.c svarlang.lib/svarlngs.lib
	upx --8086 -9 localcfg.com

deflang.c: nls_lang\*.txt
	cd nls_lang
	utf8tocp maz pl_utf8.txt > pl.txt
	..\svarlang.lib\tlumacz en pl
	move out.lng ..\localcfg.lng
	move deflang.c ..\
	cd ..

clean: .SYMBOLIC
	del *.obj
	del *.com

buildpkg: localcfg.com
	mkdir appinfo
	mkdir source
	mkdir source\localcfg
	mkdir source\localcfg\nls_lang
	mkdir bin
	mkdir doc
	mkdir nls
	copy localcfg.lsm appinfo
	copy localcfg.com bin
	copy localcfg.lng nls
	copy *.txt doc
	copy *.c source\localcfg
	copy *.h source\localcfg
	copy *.txt source\localcfg
	copy makefile source\localcfg
	copy nls_lang\*.* source\localcfg\nls_lang
	zip -9rkDX -m localcfg.zip appinfo bin doc nls source
	rmdir appinfo
	rmdir source\localcfg
	rmdir source
	rmdir bin
	rmdir doc
	rmdir nls

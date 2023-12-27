#
# localcfg Makefile for OpenWatcom
#

all: localcfg.com

localcfg.com: localcfg.c country.c deflang.c
	*wcl -0 -y -cc -wx -mt -lr -zp1 -we -d0 -ox -I=svarlang.lib localcfg.c country.c deflang.c svarlang.lib/svarlngs.lib
	del *.obj
	upx --8086 -9 localcfg.com

clean: .SYMBOLIC
	del *.obj
	del *.com

release: localcfg.com .SYMBOLIC
	mkdir appinfo
	mkdir bin
	mkdir doc
	mkdir doc\localcfg
	copy localcfg.lsm appinfo
	copy localcfg.com bin
	copy localcfg.lng bin
	copy *.txt doc\localcfg
	if exist localcfg.zip del localcfg.zip
	zip -9rkDX -m localcfg.svp appinfo bin doc
	zip -9rkDX localcfg.zip *.txt *.c *.h makefile svarlang.lib nls_lang
	rmdir appinfo
	rmdir bin
	rmdir doc\localcfg
	rmdir doc

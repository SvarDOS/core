#
# localcfg Makefile for OpenWatcom
#

all: localcfg.com

localcfg.com: localcfg.c country.c
	wcl -0 -y -cc -wx -mt -lr -zp1 -we -d0 -ox localcfg.c country.c
	upx --8086 -9 localcfg.com

clean: .SYMBOLIC
	del *.obj
	del *.com

buildpkg: localcfg.com
	mkdir appinfo
	mkdir source
	mkdir source\localcfg
	mkdir bin
	mkdir doc
	copy localcfg.lsm appinfo
	copy localcfg.com bin
	copy *.txt doc
	copy *.c source\localcfg
	copy *.h source\localcfg
	copy *.txt source\localcfg
	copy makefile source\localcfg
	zip -9rkDX -m localcfg.zip appinfo bin doc source
	rmdir appinfo
	rmdir source\localcfg
	rmdir source
	rmdir bin
	rmdir doc

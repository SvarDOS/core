#
# This is a makefile to build the SVARCOM command interpreter (COMMAND.COM)
# using OpenWatcom and nasm.
#
# You can use following targets:
#
#  wmake           - compiles the program
#  wmake clean     - cleans up all non-source files
#

FEATURES =
FEATURES += -DVERDBG

LDFLAGS = -0 -y -wx -mt -lr -we -d0 -ox -fm=command.map
CFLAGS = -0 -wx -ms -we -d0 -ox $(FEATURES)
# -0   generate 8086 compatible code
# -y   ignore %WCL% if present
# -wx  maximum warnings level
# -mt  TINY memory model
# -lr  real-mode target
# -we  any warning is considered an error
# -d0  no debug data
# -ox  maximum optimization level
#
# NOTE: wcc does not understand -mt, that is why -ms must be passed instead

all: command.com

command.com: rmodcore.h command.obj cmd.obj deflang.obj env.obj redir.obj rmodinit.obj sayonara.obj helpers.obj
	# build the final executable
	*wcl $(LDFLAGS) command.obj cmd.obj deflang.obj env.obj redir.obj rmodinit.obj sayonara.obj helpers.obj svarlang.lib\svarlngs.lib

deflang.obj: lang\*.txt
	# GENERATE CODEPAGE-SPECIFIG VERSIONS OUT OF UTF-8 FILES
	CD LANG
	utf8tocp 850 DE-UTF8.TXT > DE.TXT
	utf8tocp 437 EN-UTF8.TXT > EN.TXT
	utf8tocp 850 FR-UTF8.TXT > FR.TXT
	utf8tocp maz PL-UTF8.TXT > PL.TXT
	utf8tocp 857 TR-UTF8.TXT > TR.TXT
	..\svarlang.lib\tlumacz en de fr pl tr
	DEL ??.TXT
	MOVE /Y OUT.LNG ..\SVARCOM.LNG
	MOVE /Y DEFLANG.C ..
	CD ..
	wcc $(CFLAGS) deflang.c

cmd.obj: cmd.c cmd\*.c
	wcc $(CFLAGS) cmd.c

command.obj: command.c rmodcore.h
	wcc $(CFLAGS) command.c

helpers.obj: helpers.c
	wcc $(CFLAGS) helpers.c

.c.obj:
	wcc $(CFLAGS) $<

rmodcore.h: file2c.com rmod.bin
	file2c /s /l4096 rmod.bin rmodcore.h BUFFER

file2c.com: file2c.c
	wcl $(LDFLAGS) file2c.c

rmod.bin: rmod.asm
	nasm -f bin -l rmod.lst -o rmod.bin rmod.asm

clean: .SYMBOLIC
	del *.com
	del *.obj
	del rmod.bin
	del rmod.lst
	del rmodcore.h
	del deflang.c
	del command.map

release: command.com .SYMBOLIC
	# drop old packages if present
	IF EXIST svarcom.zip DEL svarcom.zip
	IF EXIST svarcom.svp DEL svarcom.svp
	# source package
	zip -9rkDX svarcom.zip makefile *.c *.h *.txt *.asm cmd lang svarlang.lib
	# SvarDOS binary package
	mkdir appinfo
	mkdir doc
	mkdir doc\svarcom
	mkdir nls
	copy command.com bin
	copy *.txt doc\svarcom\
	copy svarcom.lsm appinfo\
	copy svarcom.lng nls\
	upx -9 --8086 command.com
	zip -9rkDX -m svarcom.svp command.com appinfo doc nls
	rmdir appinfo
	rmdir nls
	rmdir doc\svarcom
	rmdir doc

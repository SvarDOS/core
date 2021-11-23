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

command.com: rmodcore.h command.obj cmd.obj doserr.obj env.obj redir.obj rmodinit.obj sayonara.obj helpers.obj
	*wcl $(LDFLAGS) command.obj cmd.obj doserr.obj env.obj redir.obj rmodinit.obj sayonara.obj helpers.obj

cmd.obj: cmd.c cmd\*.c
	wcc $(CFLAGS) cmd.c

helpers.obj: helpers.c deflang.h
	wcc $(CFLAGS) helpers.c

.c.obj:
	wcc $(CFLAGS) $<

rmodcore.h: file2c.com rmod.bin
	file2c /s /l4096 rmod.bin rmodcore.h BUFFER

deflang.h: file2c.com tlumacz\default.lng
	file2c /l4096 tlumacz\default.lng deflang.h langblock

file2c.com: file2c.c
	wcl $(LDFLAGS) file2c.c

rmod.bin: rmod.asm
	nasm -f bin -l rmod.lst -o rmod.bin rmod.asm

clean: .SYMBOLIC
	del *.com
	del *.obj

pkg: svarcom.zip svarcsrc.zip

svarcom.zip: command.com
	zip -9 -j svarcom.zip command.com svarcom.txt todo.txt freecom.txt tlumacz\svarcom.lng

svarcsrc.zip: .SYMBOLIC
	zip -9 -r svarcsrc.zip makefile *.c *.h *.txt *.asm tlumacz cmd

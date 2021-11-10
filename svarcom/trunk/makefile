#
# This is a makefile to build the SVARCOM command interpreter (COMMAND.COM)
# using OpenWatcom and nasm.
#
# You can use following targets:
#
#  wmake           - compiles the program
#  wmake clean     - cleans up all non-source files
#

CFLAGS = -0 -y -cc -wx -mt -lr -we -d0 -ox -fm=command.map
# -0   generate 8086 compatible code
# -y   ignore %WCL% if present
# -cc  C code
# -wx  maximum warnings level
# -mt  TINY memory model
# -lr  real-mode target
# -we  any warning is considered an error
# -d0  no debug data
# -ox  maximum optimization level

all: command.com

command.com: deflang.h rmod.h command.c cmd.c doserr.c env.c redir.c rmodinit.c helpers.c cmd\*.c
	wcl $(CFLAGS) command.c cmd.c doserr.c env.c redir.c rmodinit.c helpers.c

rmod.h: file2c.com rmod.com
	file2c /c rmod.com rmod.h rmod

deflang.h: file2c.com tlumacz\default.lng
	file2c /l4096 tlumacz\default.lng deflang.h langblock

file2c.com: file2c.c
	wcl $(CFLAGS) file2c.c

rmod.com: rmod.asm
	nasm -f bin -l rmod.lst -o rmod.com rmod.asm

clean: .SYMBOLIC
	del *.com
	del *.obj

pkg: svarcom.zip

svarcom.zip: command.com
	zip -9 svarcom.zip command.com *.txt

#
# This is a makefile to build the SvarDOS install program
#
# You can use following targets:
#
#  wmake           - compiles the program
#  wmake clean     - cleans up all non-source files
#

all: install.com

install.com: keylay.h install.c input.c video.c
	wcl -0 -y -cc -wx -mt -lr -we -d0 -ox install.c input.c video.c kitten\kitten.c
	upx --8086 -9 install.com

locales.exe: locales.c
	wcl -0 -y -cc -wx -ml -lr -we -d0 -ox locales.c

keylay.h: locales.exe
	locales.exe

clean: .SYMBOLIC
	del install.com
	del locales.exe
	del *.obj

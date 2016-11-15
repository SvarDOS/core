#
# This is a makefile to build the Svarog386 install program
#
# You can use following targets:
#
#  make           - compiles the program
#  make clean     - cleans up all non-source files
#

all: install.com

install.com: install.c cdrom.c input.c video.c
	wcl -0 -y -cc -wx -mt -lr -we -d0 -ox install.c cdrom.c input.c video.c kitten\kitten.c
	upx --8086 -9 install.com

clean: .SYMBOLIC
	del install.com
	del *.obj

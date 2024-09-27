#
# Makefile for OpenWatcom (wmake)
#

CC=wcl
CFLAGS=-ms -0 -ox -d0 -wx -we

all: tree.exe

tree.exe: tree.c stack.c
	$(CC) $(CFLAGS) tree.c stack.c

clean: .symbolic
	del tree.exe
	del *.obj

#
# Makefile for DOS 16-bit (OpenWatcom 1.9)
#

# some interesting wcc flags:
# -oz  NULL points to valid memory
# -os  favor code size over code speed
# -ox  equivalent to "-obmiler -s"   (-s removes stack overflow checks!)

CFLAGS = -j -ml -0 -bt=dos -wx -we -d0 -obmilers -dNOLFN -i=watt32\inc
LIB = watt32\lib\wattcpwl.lib

all: pkgnet.exe

pkgnet.exe: pkgnet.obj net.obj
	wcl -lr -k8192 $(LIB) *.obj -fe=pkgnet.exe

pkgnet.obj: pkgnet.c
	*wcc pkgnet.c $(CFLAGS)

net.obj: net.c
	*wcc $(CFLAGS) net.c

clean: .symbolic
	del *.obj
	del pkgnet.exe

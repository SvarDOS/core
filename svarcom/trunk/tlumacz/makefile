#
# make instructions to build tlumacz.exe with OpenWatcom
# Copyright (C) 2021 Mateusz Viste
#

all: tlumacz.exe

tlumacz.exe: tlumacz.c
	wcl -0 -y -cc -wx -mc -lr -we -ox tlumacz.c
	del *.obj

clean: .symbolic
	del *.exe
	del *.obj



all: svarcom.lng

tlumacz.exe: tlumacz.c
	wcl -0 -y -cc -wx -mc -lr -we -ox tlumacz.c
	del *.obj

svarcom.lng: tlumacz.exe nls\svarcom.??
	tlumacz en fr pl

clean: .symbolic
	del *.exe
	del *.obj
	del *.lng

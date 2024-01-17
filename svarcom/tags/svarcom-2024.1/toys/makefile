
CFLAGS = -0 -mt -wx -lr -we -ox

all: erlev.com upcase.com hackz.com fcbdir.com

erlev.com: erlev.c
	*wcl $(CFLAGS) $<

upcase.com: upcase.c
	*wcl $(CFLAGS) $<

hackz.com: hackz.c
	*wcl $(CFLAGS) $<

fcbdir.com: fcbdir.asm
	nasm fcbdir.asm -o fcbdir.com

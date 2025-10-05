

                             Mateusz' DOS Routines

                             http://mateusz.fr/mdr


Mateusz' DOS Routines (MDR) is a C library that contains a variety of routines
to ease the development of real mode DOS applications.

These routines are mostly targeted at the Open Watcom compiler, but might work
with other C compilers as well.

All the routines have been created by Mateusz Viste and are published under the
terms of the MIT license.

List of available modules:

BIOS     BIOS-based functions
COUT     console output (writing to text-mode display)
DOS      functions interacting with DOS
KEYB     basic functions to interact with the keyboard
MDA      writes directly to the MDA screen (useful for dual-screen setups)
MOUSE    mouse routines
OPL      OPL2 (Adlib style) audio
PCX      parsing, loading and uncompressing PCX images
PKTDRV   packet driver interface (sending and receiving raw Ethernet frames)
RS232    writing to and reading from an RS-232 ("COM") port
SBDIGI   playing digitized sounds with a SoundBlaster-compatible card
TIMER    high-resolution (1 kHz) timer, relies on PIT reprogramming
TRIGINT  sin and cos functions using integers only (8086-compatible)
UNZIP    iteration over ZIP archives (no decompression code)
VID12    driver for mode 12h VGA graphic (640x480, 16 colors)
VIDEO    drivers for 320x200 video modes (256 colors on VGA, 16 colors on CGA)
VIDX     driver for VGA "MODE X", ie. 320x240 with 256 colors and 3 pages
WAVE     parsing and loading WAVE sound files
XMS      detecting and using XMS memory to store data

Documentation is contained in header (*.H) files in the INC\MDR\ directory.


+============================================================================+
| USAGE                                                                      |
+============================================================================+

Using MDR is no different than using any other library: you need to include
the header file(s) you wish to rely on and pass the lib file that matches your
memory model (small/compact/medium/large) to your linker.

Example program, KBTEST.C:

  #include <mdr\dos.h>

  int main(void) {
    mdr_dos_getkey();
    return(0);
  }

How to compile with the Watcom C Compile and Link Utility:

  wcl -ms kbtest.c mdrs2024.lib


+============================================================================+
| COMPILATION FROM SOURCES                                                   |
+============================================================================+

Should you wish to compile MDR from sources instead of relying on precompiled
LIB binaries, you will need the Watcom (or Open Watcom) C compiler and use its
wmake utility as follows:

  wmake clean
  wmake model=<MEMORY MODEL>

valid memory models are:

  wmake model=s
  wmake model=c
  wmake model=m
  wmake model=l


==============================================================================


                             Mateusz' DOS Routines

                             http://mateusz.fr/mdr


Mateusz' DOS Routines (MDR) is a C library of helpers for real-mode DOS
programs: video, input, audio, timing, networking, archives and XMS.

It is written for Open Watcom (and classic Watcom C). Other DOS C compilers
may work, but are not the primary target.

Author: Mateusz Viste. License: MIT (see the source headers).

Version macros live in INC\MDR\VER.H (MDR_VER_MAJOR / MDR_VER_MINOR).


List of modules:

BIOS     BIOS helpers (notably tick counting / waiting)
COUT     text-mode console output
DOS      DOS services (keyboard, environment, Ctrl+C, paths, …)
MDA      direct writes to an MDA screen (dual-monitor debugging)
MOUSE    mouse driver (INT 33h) helpers
OPL      OPL2 / AdLib FM synthesis and IMF playback
PCX      PCX header analysis, RLE decode and bit-depth unpack
PKTDRV   packet-driver Ethernet (raw frames)
RS232    COM / RS-232 I/O
SBDIGI   Sound Blaster digitized PCM playback (DMA)
TIMER    high-resolution timer (~1.1 kHz) via PIT reprogramming
TRIGINT  integer-only sin/cos (8086-friendly, no FPU)
UNZIP    ZIP archive iteration and access to stored (uncompressed) members
VID12    VGA mode 12h (640x480, 16 colors)
VIDEO    320x200 drivers (VGA 256 colors / CGA 16 colors)
VIDX     VGA Mode X (320x240, 256 colors, 3 pages)
WAVE     WAVE / RIFF PCM parsing
XMS      XMS detection, allocation and block moves

API documentation is in the headers under INC\MDR\. HISTORY.TXT lists changes
between releases.


+============================================================================+
| REQUIREMENTS                                                               |
+============================================================================+

  - Real-mode DOS (or a compatible environment such as DOSBox / DOSEMU)
  - Open Watcom C (wcc / wcl / wmake), targeting DOS
  - One of the supported memory models: small, compact, medium or large

MDR is not intended for protected-mode extenders or Win32.


+============================================================================+
| USAGE                                                                      |
+============================================================================+

Include the headers you need and link the library built for your memory model.
Put INC on the compiler include path so #include <mdr\...> resolves.

Example (KBTEST.C):

  #include <mdr\dos.h>

  int main(void) {
    mdr_dos_getkey();
    return(0);
  }

Compile and link with Open Watcom (small model, 2026 library name):

  wcl -ms -i=inc kbtest.c mdrs2026.lib

Prebuilt library names follow mdr<model><year>.lib:

  mdrs2026.lib   small
  mdrc2026.lib   compact
  mdrm2026.lib   medium
  mdrl2026.lib   large

Use the year that matches the MDR release you linked against.


+============================================================================+
| COMPILATION FROM SOURCES                                                   |
+============================================================================+

From the MDR trunk directory, with Open Watcom's wmake:

  wmake clean
  wmake model=<MEMORY MODEL>

Valid models:

  wmake model=s
  wmake model=c
  wmake model=m
  wmake model=l

This produces mdr<model>2026.lib in the same directory (year comes from the
makefile). Object files for all modules are built into that single library.


+============================================================================+
| IMPORTANT NOTES                                                            |
+============================================================================+

TIMER
  mdr_timer_init() reprograms the system PIT. Always call mdr_timer_stop()
  before exit (atexit is fine), or DOS timing will be left in a bad state.

SBDIGI
  Samples must be unsigned 8-bit mono PCM at 8 kHz. Pass a BLASTER-style
  string such as "A220 I5 D1". Playback returns while DMA is still running.
  MDR does not allocate the sample buffer: you must supply memory that does
  not cross a 64K physical page (ISA DMA cannot cross that boundary).

XMS
  mdr_xms_push() / mdr_xms_pull() require an even byte count (XMS move API).

PKTDRV
  Call mdr_pktdrv_init() successfully before any other pktdrv function.

UNZIP
  Walks the archive and exposes each member's offset and size so you can read
  stored (method 0) payloads directly from the file. Deflate and other
  compressed methods are not inflated by MDR.

OPL / IMF
  IMF playback needs a regularly advancing 16-bit clock and frequent polling;
  see INC\MDR\OPL.H.

Headers are the authoritative reference for each function's contract.


+============================================================================+
| TESTS                                                                      |
+============================================================================+

Each module has a test program under TESTS\ (t_bios.exe, t_cout.exe, ...).
Build the library first, then the tests:

  wmake model=c
  cd tests
  wmake

Run a test on real-mode DOS (or DOSBox). Functions that cannot be checked
in software (sound, mouse feel, graphics) ask y/n. Pass -a to skip prompts
while still calling the functions:

  t_opl.exe
  t_opl.exe -a

TESTS\COVERAGE.TXT lists every public function and which test calls it.
Regenerate it on a host with Python:

  python3 tests\mkcover.py


==============================================================================

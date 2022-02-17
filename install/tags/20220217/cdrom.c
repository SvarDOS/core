/*
 * CD-ROM detection routines
 * Copyright (C) 2016 Mateusz Viste
 */

#include <dos.h>
#include "cdrom.h" /* include self for control */

/* returns 1 if drive drv is a valid CDROM, zero if not, negative if no MSCDEX (0=A:, etc) */
int cdrom_drivecheck(int drv) {
  union REGS r;
  r.x.ax = 0x150B;
  r.x.cx = drv;
  int86(0x2F, &r, &r);
  if (r.x.bx != 0xADAD) return(-1); /* look for the MSCDEX signature */
  if (r.x.ax == 0) return(0);
  return(1);
}

/* returns the identifier of the first CDROM drive (0=A:, etc), or a negative value on error */
int cdrom_findfirst(void) {
  int i;
  for (i = 2; i < 26; i++) { /* check drives from C to Z */
    int cdres = cdrom_drivecheck(i);
    if (cdres == 0) continue;
    if (cdres == 1) return(i);
    break;
  }
  return(-1);
}

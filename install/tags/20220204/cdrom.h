/*
 * CD-ROM detection routines
 * Copyright (C) 2016 Mateusz Viste
 */

#ifndef CDROM_H_SENTINEL
#define CDROM_H_SENTINEL

/* returns 1 if drive drv is a valid CDROM, zero if not, negative if no MSCDEX (0=A:, etc) */
int cdrom_drivecheck(int drv);

/* returns the identifier of the first CDROM drive (0=A:, etc), or a negative value on error */
int cdrom_findfirst(void);

#endif

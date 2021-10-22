/*
 * translates a DOS extended error into a human string
 * (as defined by INT 21/AH=59h/BX=0000h)
 */

#ifndef DOSERR_H
#define DOSERR_H

const char *doserr(unsigned short err);

#endif

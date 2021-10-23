#ifndef RMODINIT_H
#define RMODINIT_H

#define RMOD_OFFSET_ENVSEG     0x08
#define RMOD_OFFSET_LEXITCODE  0x0A
#define RMOD_OFFSET_INPBUFF    0x0C
#define RMOD_OFFSET_COMSPECPTR 0x8E
#define RMOD_OFFSET_BOOTDRIVE  0x90
#define RMOD_OFFSET_ROUTINE    0x9F

unsigned short rmod_install(unsigned short envsize);
unsigned short rmod_find(void);
void rmod_updatecomspecptr(unsigned short rmod_seg, unsigned short env_seg);

#endif

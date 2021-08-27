/*
 * unpacks a http "chunked" transfer into a raw data stream.
 * this file is part of the pkgnet tool from the SvarDOS project.
 *
 * Copyright (C) 2021 Mateusz Viste
 */

#ifndef UNCHUNK_H
#define UNCHUNK_H

/* transforms a http CHUNKED stream into actual data, returns the amount of raw data to read */
int unchunk(unsigned char *buff, int bufflen);

#endif

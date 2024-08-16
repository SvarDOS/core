/*
 * This file is part of pkginst
 * Copyright (C) 2012-2024 Mateusz Viste
 *
 * It contains a few helper function...
 */

#ifndef helpers_sentinel
#define helpers_sentinel

#include "loadconf.h"   /* required for the customdirs struct */

/* outputs a NUL-terminated string to stdout */
void output(const char *s);

void slash2backslash(char *str);

/* trim CRC from a filename and returns a pointer to the CRC part.
 * this is used to parse filename lines from LSM files */
char *trimfnamecrc(char *fname);

char *fdnpkg_strcasestr(const char *s, const char *find);

/* recursively mkdir() directories of a path.
 * ignores the trailing filename if there is one */
void mkpath(char *dirs);

char *computelocalpath(char *longfilename, char *respath, const char *dosdir, const struct customdirs *dirlist, char bootdrive);
void removeDoubleBackslashes(char *str);
char *getfext(char *fname);
int freadtokval(FILE *fd, char *line, size_t maxlen, char **val, char delim);

#endif

/*
 * This file is part of pkginst
 * Copyright (C) 2012-2021 Mateusz Viste
 *
 * It contains a few helper function...
 */

#ifndef helpers_sentinel
#define helpers_sentinel
#include "loadconf.h"   /* required for the customdirs struct */
int isversionnewer(char *v1, char *v2);
void slash2backslash(char *str);
void backslash2slash(char *str);
void strtolower(char *mystring);
char *fdnpkg_strcasestr(const char *s, const char *find);
void mkpath(char *dirs);
char *computelocalpath(char *longfilename, char *respath, const char *dosdir, const struct customdirs *dirlist);
void removeDoubleBackslashes(char *str);
int detect_localpath(char *url);
char *getfext(char *fname);
#endif

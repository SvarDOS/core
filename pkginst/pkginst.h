/*
 *  This file is part of SvarDOS
 *  Copyright (C) 2012-2021 Mateusz Viste
 */

#ifndef pkginst_sentinel
#define pkginst_sentinel

#include "loadconf.h" /* required for struct customdirs */

int is_package_installed(char *pkgname, char *dosdir);
struct ziplist *pkginstall_preparepackage(char *pkgname, char *localfile, int nosourceflag, FILE **zipfd, char *dosdir, struct customdirs *dirlist, char *buffmem1k);
int pkginstall_installpackage(char *pkgname, char *dosdir, struct customdirs *dirlist, struct ziplist *ziplinkedlist, FILE *zipfd);
int validate_package_not_installed(char *pkgname, char *dosdir);

#endif

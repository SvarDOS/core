/*
 *  This file is part of FDNPKG
 *  Copyright (C) 2012-2017 Mateusz Viste
 */

#ifndef pkginst_sentinel
#define pkginst_sentinel

#include "pkgdb.h"
#include "loadconf.h" /* required for struct customdirs */

int is_package_installed(char *pkgname, char *dosdir, char *mapdrv);
struct ziplist *pkginstall_preparepackage(struct pkgdb *pkgdb, char *pkgname, char *tempdir, char *localfile, int nosourceflag, char **repolist, FILE **zipfd, char *proxy, int proxyport, char *downloadingstring, char *dosdir, struct customdirs *dirlist, char *buffmem1k, char *mapdrv);
int pkginstall_installpackage(char *pkgname, char *dosdir, struct customdirs *dirlist, struct ziplist *ziplinkedlist, FILE *zipfd, char *mapdrv);
int validate_package_not_installed(char *pkgname, char *dosdir, char *mapdrv);

#endif

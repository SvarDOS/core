/*
 *  This file is part of SvarDOS
 *  Copyright (C) 2012-2021 Mateusz Viste
 */

#ifndef pkginst_sentinel
#define pkginst_sentinel

#include "loadconf.h" /* required for struct customdirs */

#define PKGINST_SKIPLINKS 1
#define PKGINST_UPDATE    2

int is_package_installed(const char *pkgname, const char *dosdir);
struct ziplist *pkginstall_preparepackage(const char *pkgname, const char *localfile, int flags, FILE **zipfd, const char *dosdir, const struct customdirs *dirlist);
int pkginstall_installpackage(const char *pkgname, const char *dosdir, const struct customdirs *dirlist, struct ziplist *ziplinkedlist, FILE *zipfd);

#endif

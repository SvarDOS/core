/*
 * This file is part of FDNPKG
 * Copyright (C) 2013-2017 Mateusz Viste
 */

#include "fdnpkg.h"
#include "pkgdb.h"

#ifndef showinst_h_sentinel
  #define showinst_h_sentinel
  void pkg_freeflist(struct flist_t *flist);
  struct flist_t *pkg_loadflist(char *pkgname, char *dosdir);
  void showinstalledpkgs(char *filterstr, char *dosdir);
  int checkupdates(char *dosdir, struct pkgdb *pkgdb, char **repolist, char *pkg, char *tempdir, int flags, struct customdirs *dirlist, char *proxy, int proxyport, char *downloadingstring, char *mapdrv);
  void listfilesofpkg(char *pkgname, char *dosdir);
#endif

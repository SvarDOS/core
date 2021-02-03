/*
 * This file is part of pkg (SvarDOS)
 * Copyright (C) 2013-2021 Mateusz Viste
 */

#ifndef showinst_h_sentinel
  #define showinst_h_sentinel

  struct flist_t {
    struct flist_t *next;
    char fname[1]; /* this must be the last item in the structure, it will be expanded to fit the filename */
  };

  void pkg_freeflist(struct flist_t *flist);
  struct flist_t *pkg_loadflist(const char *pkgname, const char *dosdir);
  int showinstalledpkgs(const char *filterstr, const char *dosdir);
  int listfilesofpkg(const char *pkgname, const char *dosdir);
#endif

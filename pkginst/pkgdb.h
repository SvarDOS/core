/*
 * Package database manipulation routines.
 * This file is part of the FDNPKG project.
 *
 * Copyright (C) 2012-2017 Mateusz Viste
 */

#ifndef pkgdb_sentinel
#define pkgdb_sentinel

struct pkgrepo {
  unsigned char repo;
  char version[16];
  unsigned long crc32zip;
  unsigned long crc32zib;
  struct pkgrepo *nextrepo;
};

struct pkgdb {
  char name[9];
  char *desc;  /* the description of the package - will be strdup()ed when time will come */
  struct pkgrepo *repolist;
  struct pkgdb *nextpkg;
};


struct pkgdb *createdb(void);
void freedb(struct pkgdb **db);
struct pkgdb *findpkg(struct pkgdb *db, char *pkgname, struct pkgdb **lastmatch);
int loaddb(struct pkgdb *db, char *datafile, unsigned char repo, char **dbmsg);
int loaddb_fromcache(struct pkgdb *db, char *datafile, unsigned long crc32val, long maxcachetime);
void dumpdb(struct pkgdb *db, char *datafile, unsigned long crc32val);

#endif

/*
 * This file is part of FDNPKG.
 *
 * Copyright (C) 2012-2016 Mateusz Viste
 */

#ifndef loadrepolist_sentinel
#define loadrepolist_sentinel

struct customdirs {
  char *name;
  char *location;
  struct customdirs *next;
};

/* Loads the list of repositories from the config file specified in %FDNPKG%.
 * Returns the amount of repositories found (and loaded) on success, or -1 on failure. */
int loadconf(char *cfgfile, char **repolist, int maxreps, unsigned long *crc32val, long *maxcachetime, struct customdirs **dirlist, int *nosourceflag, char **proxy, int *proxyport, char **mapdrv);

/* Free the memory allocated at configuration load. */
void freeconf(char **repolist, int repscount, struct customdirs **dirlist);

#endif

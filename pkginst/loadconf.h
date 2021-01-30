/*
 * This file is part of pkginst.
 *
 * Copyright (C) 2012-2021 Mateusz Viste
 */

#ifndef loadrepolist_sentinel
#define loadrepolist_sentinel

struct customdirs {
  struct customdirs *next;
  char name[9];
  char location[1]; /* extended at alloc time */
};

/* Loads the list of custom directories from the config file
 * Returns 0 on success, or -1 on failure. */
int loadconf(const char *dosdir, struct customdirs **dirlist, int *flags);

/* Free the memory allocated at configuration load. */
void freeconf(struct customdirs **dirlist);

#endif

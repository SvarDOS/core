/*
 * This file is part of pkginst (SvarDOS).
 *
 * Loads the list of repositories from a config file.
 *
 * Copyright (C) 2012-2021 Mateusz Viste
 */

#include <stdio.h>  /* printf(), fclose(), fopen()... */
#include <string.h> /* strcasecmp() */
#include <stdlib.h> /* malloc(), free() */

#include "pkginst.h" /* PKGINST_SKIPLINKS... */
#include "helpers.h" /* slash2backslash(), removeDoubleBackslashes()... */
#include "kprintf.h"
#include "loadconf.h"
#include "parsecmd.h"


void freeconf(struct customdirs **dirlist) {
  struct customdirs *curpos;
  /* free the linked list of custom dirs */
  while (*dirlist != NULL) {
    curpos = *dirlist;
    *dirlist = (*dirlist)->next;
    free(curpos);
  }
}


static int checkfordoubledirlist(const struct customdirs *dirlist) {
  struct customdirs *curpos;
  for (; dirlist != NULL; dirlist = dirlist->next) {
    for (curpos = dirlist->next; curpos != NULL; curpos = curpos->next) {
      if (strcasecmp(curpos->name, dirlist->name) == 0) {
        kitten_printf(7, 0, "Error: custom dir '%s' is listed twice!", curpos->name);
        puts("");
        return(-1);
      }
    }
  }
  return(0);
}


/* validates dirlist entries: check that they are absolute paths and are not using restricted names */
static int validatedirlist(const struct customdirs *dirlist) {
  for (; dirlist != NULL; dirlist = dirlist->next) {
    /* the location must be at least 3 characters long to be a valid absolute path (like 'c:\')*/
    if (strlen(dirlist->location) < 3) {
      kitten_printf(7, 15, "Error: custom dir '%s' is not a valid absolute path!", dirlist->name);
      puts("");
      return(-1);
    }
    /* is it a valid absolute path? should start with [a..Z]:\ */
    if ((dirlist->location[1] != ':') ||
       ((dirlist->location[2] != '/') && (dirlist->location[2] != '\\')) ||
       (((dirlist->location[0] < 'a') || (dirlist->location[0] > 'z')) && ((dirlist->location[0] < 'A') || (dirlist->location[0] > 'Z')))) {
      kitten_printf(7, 15, "Error: custom dir '%s' is not a valid absolute path!", dirlist->name);
      puts("");
      return(-1);
    }
    /* check for forbidden names */
    if ((strcasecmp(dirlist->name, "appinfo") == 0) ||
        (strcasecmp(dirlist->name, "bin") == 0) ||
        (strcasecmp(dirlist->name, "doc") == 0) ||
        (strcasecmp(dirlist->name, "help") == 0) ||
        (strcasecmp(dirlist->name, "nls") == 0) ||
        (strcasecmp(dirlist->name, "packages") == 0)) {
      kitten_printf(7, 16, "Error: custom dir '%s' is a reserved name!", dirlist->name);
      puts("");
      return(-1);
    }
  }
  return(0);
}


/* add (and allocates) a new custom dir entry to dirlist. Returns 0 on success,
   or non-zero on failure (failures happen on out of memory events). */
static int addnewdir(struct customdirs **dirlist, const char *name, const char *location) {
  struct customdirs *newentry;
  if (strlen(name) >= sizeof(newentry->name)) return(-2);
  newentry = malloc(sizeof(struct customdirs) + strlen(location) + 1);
  if (newentry == NULL) return(-1);
  strcpy(newentry->name, name);
  strcpy(newentry->location, location);
  newentry->next = *dirlist;
  *dirlist = newentry;
  return(0);
}


int loadconf(const char *dosdir, struct customdirs **dirlist, int *flags) {
  FILE *fd;
  char *value = NULL;
  char token[512];
  int nline = 0;

  snprintf(token, sizeof(token), "%s\\cfg\\pkginst.cfg", dosdir);
  fd = fopen(token, "r");
  if (fd == NULL) {
    kitten_printf(7, 1, "Error: Could not open config file (%s)!", token);
    puts("");
    return(-1);
  }

  /* read the config file line by line */
  while (freadtokval(fd, token, sizeof(token), &value, ' ') == 0) {
    nline++;

    /* skip comments and empty lines */
    if ((token[0] == '#') || (token[0] == 0)) continue;

    if ((value == NULL) || (value[0] == 0)) {
      kitten_printf(7, 4, "Warning: token with empty value on line #%d", nline);
      puts("");
      continue;
    }

    /* printf("token='%s' ; value = '%s'\n", token, value); */
    if (strcasecmp(token, "SKIPLINKS") == 0) {
      int tmpint = atoi(value); /* must be 0/1 */
      if (tmpint == 0) {
        /* do nothing */
      } else if (tmpint == 1) {
        *flags |= PKGINST_SKIPLINKS;
      } else {
        kitten_printf(7, 10, "Warning: Ignored an illegal '%s' value at line #%d", "skiplinks", nline);
        puts("");
      }
    } else if (strcasecmp(token, "DIR") == 0) { /* custom directory entry */
      char *argv[2];
      if (parsecmd(value, argv, 2) != 2) {
        kitten_printf(7, 11, "Warning: Invalid 'DIR' directive found at line #%d", nline);
        puts("");
      }
      /* add the entry to the list */
      slash2backslash(argv[1]);
      removeDoubleBackslashes(argv[1]);
      if (argv[1][strlen(argv[1]) - 1] != '\\') strcat(argv[1], "\\"); /* make sure to end dirs with a backslash */
      if (addnewdir(dirlist, argv[0], argv[1]) != 0) {
        kitten_printf(2, 14, "Out of memory! (%s)", "addnewdir");
        puts("");
        freeconf(dirlist);
        fclose(fd);
        return(-1);
      }
    } else { /* unknown token */
      kitten_printf(7, 8, "Warning: Unknown token '%s' at line #%d", token, nline);
      puts("");
    }
  }
  fclose(fd);

  /* perform some validations */
  if ((checkfordoubledirlist(*dirlist) != 0) || (validatedirlist(*dirlist) != 0)) {
    freeconf(dirlist);
    return(-1);
  }

  return(0);
}

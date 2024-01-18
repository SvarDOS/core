/*
 * This file is part of pkg (SvarDOS).
 * Copyright (C) 2012-2024 Mateusz Viste
 */

#include <stdio.h>  /* printf(), fclose(), fopen()... */
#include <string.h> /* strcasecmp() */
#include <stdlib.h> /* malloc(), free() */

#include "helpers.h" /* slash2backslash(), removeDoubleBackslashes()... */
#include "kprintf.h"

#include "loadconf.h"


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
        kitten_printf(7, 0, curpos->name); /* "ERROR: custom dir '%s' is listed twice!" */
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
      kitten_printf(7, 15, dirlist->name); /* "ERROR: custom dir '%s' is not a valid absolute path!" */
      puts("");
      return(-1);
    }
    /* is it a valid absolute path? should start with [a..Z]:\ */
    if ((dirlist->location[1] != ':') ||
       ((dirlist->location[2] != '/') && (dirlist->location[2] != '\\')) ||
       (((dirlist->location[0] < 'a') || (dirlist->location[0] > 'z')) && ((dirlist->location[0] < 'A') || (dirlist->location[0] > 'Z')))) {
      kitten_printf(7, 15, dirlist->name); /* "ERROR: custom dir '%s' is not a valid absolute path!" */
      puts("");
      return(-1);
    }
    /* check for forbidden names */
    if ((strcasecmp(dirlist->name, "appinfo") == 0) ||
        (strcasecmp(dirlist->name, "doc") == 0) ||
        (strcasecmp(dirlist->name, "help") == 0) ||
        (strcasecmp(dirlist->name, "nls") == 0) ||
        (strcasecmp(dirlist->name, "packages") == 0)) {
      kitten_printf(7, 16, dirlist->name); /* "ERROR: custom dir '%s' is a reserved name!" */
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


int loadconf(const char *dosdir, struct customdirs **dirlist) {
  FILE *fd;
  char *value = NULL;
  char token[512];
  int nline = 0;

  snprintf(token, sizeof(token), "%s\\cfg\\pkg.cfg", dosdir);
  fd = fopen(token, "r");
  if (fd == NULL) {
    kitten_printf(7, 1, token); /* "ERROR: Could not open config file (%s)!" */
    puts("");
    return(-1);
  }

  *dirlist = NULL;

  /* read the config file line by line */
  while (freadtokval(fd, token, sizeof(token), &value, ' ') == 0) {
    nline++;

    /* skip comments and empty lines */
    if ((token[0] == '#') || (token[0] == 0)) continue;

    if ((value == NULL) || (value[0] == 0)) {
      kitten_printf(7, 4, nline); /* "Warning: token with empty value on line #%d" */
      puts("");
      continue;
    }

    /* printf("token='%s' ; value = '%s'\n", token, value); */
    if (strcasecmp(token, "DIR") == 0) { /* custom directory entry */
      char *location = NULL;
      int i;
      /* find nearer space */
      for (i = 0; (value[i] != ' ') && (value[i] != 0); i++);
      if (value[i] == 0) {
        kitten_printf(7, 11, nline); /* "Warning: Invalid 'DIR' directive found at line #%d" */
        puts("");
        continue;
      }
      value[i] = 0;
      location = value + i + 1;

      /* add the entry to the list */
      slash2backslash(location);
      removeDoubleBackslashes(location);
      if (location[strlen(location) - 1] != '\\') strcat(location, "\\"); /* make sure to end dirs with a backslash */
      if (addnewdir(dirlist, value, location) != 0) {
        kitten_printf(2, 14, "addnewdir"); /* "Out of memory! (%s)" */
        puts("");
        freeconf(dirlist);
        fclose(fd);
        return(-1);
      }
    } else { /* unknown token */
      kitten_printf(7, 8, token, nline); /* "Warning: Unknown token '%s' at line #%d" */
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

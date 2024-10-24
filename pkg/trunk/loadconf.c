/*
 * This file is part of pkg (SvarDOS).
 * Copyright (C) 2012-2024 Mateusz Viste
 */

#include <stdio.h>  /* printf(), fclose(), fopen()... */
#include <string.h>
#include <strings.h> /* strcasecmp() */
#include <stdlib.h> /* malloc(), free() */

#include "helpers.h" /* slash2backslash(), removeDoubleBackslashes()... */
#include "svarlang.lib/svarlang.h"

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
        char msg[256];
        sprintf(msg, svarlang_str(7, 0), curpos->name); /* "ERROR: custom dir '%s' is listed twice!" */
        output(msg);
        outputnl(" (PKG.CFG)");
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
    if (strlen(dirlist->location) < 3) goto INVALID_CUSTOM_DIR;

    /* is it a valid absolute path? should start with [a..Z]:\ */
    if ((dirlist->location[1] != ':') ||
       ((dirlist->location[2] != '/') && (dirlist->location[2] != '\\')) ||
       (((dirlist->location[0] < 'a') || (dirlist->location[0] > 'z')) && ((dirlist->location[0] < 'A') || (dirlist->location[0] > 'Z')))) {
      goto INVALID_CUSTOM_DIR;
    }

    /* check for forbidden names */
    if ((strcasecmp(dirlist->name, "appinfo") == 0) ||
        (strcasecmp(dirlist->name, "doc") == 0) ||
        (strcasecmp(dirlist->name, "help") == 0) ||
        (strcasecmp(dirlist->name, "nls") == 0) ||
        (strcasecmp(dirlist->name, "packages") == 0)) {
      goto INVALID_CUSTOM_DIR;
    }
  }
  return(0);

  INVALID_CUSTOM_DIR:
  output(svarlang_str(7, 15)); /* "ERROR: invalid custom dir:" */
  output(" ");
  output(dirlist->name);
  outputnl(" (PKG.CFG)");
  return(-1);
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


int loadconf(const char *dosdir, struct customdirs **dirlist, char *bootdrive) {
  FILE *fd;
  char *value = NULL;
  char token[512];
  int nline = 0;
  const char *PKG_CFG = " (PKG.CFG)";
  char msg[256];

  /* load config file from %DOSDIR%\PKG.CFG */
  snprintf(token, sizeof(token), "%s\\pkg.cfg", dosdir);
  fd = fopen(token, "r");
  /* if not found then try the legacy location at %DOSDIR%\CFG\PKG.CFG */
  if (fd == NULL) {
    snprintf(token, sizeof(token), "%s\\cfg\\pkg.cfg", dosdir);
    fd = fopen(token, "r");
    if (fd == NULL) {
      sprintf(msg, svarlang_str(7, 1), "%DOSDIR%\\PKG.CFG"); /* "ERROR: Could not open config file (%s)!" */
      outputnl(msg);
    } else {
      sprintf(msg, svarlang_str(7, 17), token);  /* "ERROR: PKG.CFG found at %s */
      outputnl(msg);
      outputnl(svarlang_str(7, 18));    /* Please move it to %DOSDIR%\PKG.CFG */
    }
    return(-1);
  }

  *dirlist = NULL;
  *bootdrive = 'C'; /* default */

  /* read the config file line by line */
  while (freadtokval(fd, token, sizeof(token), &value, ' ') == 0) {
    nline++;

    /* skip comments and empty lines */
    if ((token[0] == '#') || (token[0] == 0)) continue;

    if ((value == NULL) || (value[0] == 0)) {
      sprintf(msg, svarlang_str(7, 4), nline); /* "Warning: token with empty value on line #%d" */
      output(msg);
      outputnl(PKG_CFG);
      continue;
    }

    /* printf("token='%s' ; value = '%s'\n", token, value); */
    if (strcasecmp(token, "DIR") == 0) { /* custom directory entry */
      char *location = NULL;
      int i;
      /* find nearer space */
      for (i = 0; (value[i] != ' ') && (value[i] != 0); i++);
      if (value[i] == 0) {
        sprintf(msg, svarlang_str(7, 11), nline); /* "Warning: Invalid 'DIR' directive found at line #%d" */
        output(msg);
        outputnl(PKG_CFG);
        continue;
      }
      value[i] = 0;
      location = value + i + 1;

      /* add the entry to the list */
      slash2backslash(location);
      removeDoubleBackslashes(location);
      if (location[strlen(location) - 1] != '\\') strcat(location, "\\"); /* make sure to end dirs with a backslash */
      if (addnewdir(dirlist, value, location) != 0) {
        outputnl(svarlang_str(2, 14)); /* "Out of memory!" */
        freeconf(dirlist);
        fclose(fd);
        return(-1);
      }
    } else if (strcasecmp(token, "BOOTDRIVE") == 0) { /* boot drive (used for kernel and command.com installation */
      *bootdrive = value[0];
      *bootdrive &= 0xDF; /* upcase it */
      if ((*bootdrive < 'A') || (*bootdrive > 'Z')) {
        sprintf(msg, svarlang_str(7, 5), nline); /* Warning: Invalid bootdrive at line #%d */
        output(msg);
        outputnl(PKG_CFG);
        *bootdrive = 'C';
      }
    } else { /* unknown token */
      sprintf(msg, svarlang_str(7, 8), token, nline); /* "Warning: Unknown token '%s' at line #%d" */
      output(msg);
      outputnl(PKG_CFG);
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

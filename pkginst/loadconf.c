/*
 * This file is part of FDNPKG.
 *
 * Loads the list of repositories from the config file specified in %FDNPKG%.
 * Returns the amount of repositories found (and loaded) on success, or -1 on failure.
 *
 * Copyright (C) 2012-2016 Mateusz Viste
 */

#include <stdio.h>  /* printf(), fclose(), fopen()... */
#include <string.h> /* strcasecmp() */
#include <stdlib.h> /* malloc(), free() */

#include "crc32.h"  /* crc32() */
#include "fdnpkg.h" /* PKGINST_SKIPLINKS... */
#include "helpers.h" /* slash2backslash(), removeDoubleBackslashes()... */
#include "kprintf.h"
#include "loadconf.h"
#include "parsecmd.h"
#include "version.h"


void freeconf(struct customdirs **dirlist) {
  struct customdirs *curpos;
  /* free the linked list of custom dirs */
  while (*dirlist != NULL) {
    curpos = *dirlist;
    if (curpos->name != NULL) free(curpos->name);
    if (curpos->location != NULL) free(curpos->location);
    *dirlist = (*dirlist)->next;
    free(curpos);
  }
  *dirlist = NULL;
}


static int checkfordoubledirlist(struct customdirs *dirlist) {
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
static int validatedirlist(struct customdirs *dirlist) {
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
static int addnewdir(struct customdirs **dirlist, char *name, char *location) {
  struct customdirs *newentry;
  newentry = malloc(sizeof(struct customdirs));
  if (newentry == NULL) return(-1);
  newentry->name = malloc(strlen(name) + 1);
  if (newentry->name == NULL) {
    free(newentry);
    return(-1);
  }
  newentry->location = malloc(strlen(location) + 1);
  if (newentry->location == NULL) {
    free(newentry->name);
    free(newentry);
    return(-1);
  }
  strcpy(newentry->name, name);
  strcpy(newentry->location, location);
  newentry->next = *dirlist;
  *dirlist = newentry;
  return(0);
}


int loadconf(char *cfgfile, struct customdirs **dirlist, int *flags) {
  int bytebuff, parserstate = 0;
  FILE *fd;
  #define maxtok 16
  char token[maxtok];
  #define maxval 1024
  char value[maxval];
  int curtok = 0, curval = 0, nline = 1;

  fd = fopen(cfgfile, "r");
  if (fd == NULL) {
    kitten_printf(7, 1, "Error: Could not open config file '%s'!", cfgfile);
    puts("");
    return(-1);
  }

  /* read the config file line by line */
  do {
    bytebuff = fgetc(fd);
    if (bytebuff != '\r') {
      switch (parserstate) {
        case 0: /* Looking for start of line */
          if ((bytebuff == EOF) || (bytebuff == ' ') || (bytebuff == '\t')) break;
          if (bytebuff == '\n') {
            nline += 1;
            break;
          }
          if (bytebuff == '#') {
              parserstate = 9;
            } else {
              token[0] = bytebuff;
              curtok = 1;
              parserstate = 1;
          }
          break;
        case 1: /* Looking for token end */
          if ((bytebuff == EOF) || (bytebuff == '\n')) {
            kitten_printf(7, 2, "Warning: token without value on line #%d", nline);
            puts("");
            if (bytebuff == '\n') nline += 1;
            parserstate = 0;
            break;
          }
          if ((bytebuff == ' ') || (bytebuff == '\t')) {
              token[curtok] = 0;
              parserstate = 2;
            } else {
              token[curtok] = bytebuff;
              curtok += 1;
              if (curtok >= maxtok) {
                parserstate = 9; /* ignore the line */
                kitten_printf(7, 3, "Warning: Config file token overflow on line #%d", nline);
                puts("");
              }
          }
          break;
        case 2: /* Looking for value start */
          if ((bytebuff == EOF) || (bytebuff == '\n')) {
            kitten_printf(7, 4, "Warning: token with empty value on line #%d", nline);
            puts("");
            if (bytebuff == '\n') nline += 1;
            parserstate = 0;
            break;
          }
          if ((bytebuff != ' ') && (bytebuff != '\t')) {
            value[0] = bytebuff;
            curval = 1;
            parserstate = 3;
          }
          break;
        case 3: /* Looking for value end */
          if ((bytebuff == EOF) || (bytebuff == '\n')) {
              parserstate = 0;
              value[curval] = 0;
              if ((value[curval - 1] == ' ') || (value[curval - 1] == '\t')) {
                kitten_printf(7, 5, "Warning: trailing white-space(s) after value on line #%d", nline);
                puts("");
                while ((value[curval - 1] == ' ') || (value[curval - 1] == '\t')) value[--curval] = 0;
              }
              /* Interpret the token/value pair now! */
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
                } else if (strcasecmp(token, "DIR") == 0) { /* custom repository entry */
                  char *argv[2], *evar, *evar_content, *realLocation;
                  #define realLocation_len 512
                  int x, y;
                  if (parsecmd(value, argv, 2) != 2) {
                    kitten_printf(7, 11, "Warning: Invalid 'DIR' directive found at line #%d", nline);
                    puts("");
                  }
                  realLocation = malloc(realLocation_len);
                  if (realLocation == NULL) {
                    kitten_printf(2, 14, "Out of memory! (%s)", "malloc realLocation");
                    puts("");
                    freeconf(dirlist);
                    fclose(fd);
                    return(-1);
                  }
                  realLocation[0] = 0; /* force it to be empty, since we might use strcat() on this later! */
                  /* resolve possible env variables */
                  evar = NULL;
                  y = 0;
                  for (x = 0; argv[1][x] != 0; x++) {
                    if (evar == NULL) { /* searching for % and copying */
                        if (argv[1][x] == '%') {
                            evar = &argv[1][x+1];
                          } else {
                            if (y + 1 > realLocation_len) {
                              kitten_printf(7, 12, "Error: DIR path too long at line #%d", nline);
                              puts("");
                              freeconf(dirlist);
                              free(realLocation);
                              fclose(fd);
                              return(-1);
                            }
                            realLocation[y] = argv[1][x]; /* copy over */
                            y++;
                            realLocation[y] = 0; /* make sure to terminate the string at any time */
                        }
                      } else { /* reading a % variable */
                        if (argv[1][x] == '%') {
                          argv[1][x] = 0;
                          evar_content = getenv(evar);
                          if (evar_content == NULL) {
                            kitten_printf(7, 13, "Error: Found inexisting environnement variable '%s' at line #%d", evar, nline);
                            puts("");
                            freeconf(dirlist);
                            free(realLocation);
                            fclose(fd);
                            return(-1);
                          }
                          if (strlen(evar_content) + y + 1 > realLocation_len) {
                            kitten_printf(7, 12, "Error: DIR path too long at line #%d", nline);
                            puts("");
                            freeconf(dirlist);
                            free(realLocation);
                            fclose(fd);
                            return(-1);
                          }
                          strcat(realLocation, evar_content);
                          y += strlen(evar_content);
                          evar = NULL;
                        }
                    }
                  }
                  /* add the entry to the list */
                  slash2backslash(realLocation);
                  removeDoubleBackslashes(realLocation);
                  if (realLocation[strlen(realLocation) - 1] != '\\') strcat(realLocation, "\\"); /* make sure to end dirs with a backslash */
                  if (addnewdir(dirlist, argv[0], realLocation) != 0) {
                    kitten_printf(2, 14, "Out of memory! (%s)", "addnewdir");
                    puts("");
                    freeconf(dirlist);
                    free(realLocation);
                    fclose(fd);
                    return(-1);
                  }
                  free(realLocation);
                } else { /* unknown token */
                  kitten_printf(7, 8, "Warning: Unknown token '%s' at line #%d", token, nline);
                  puts("");
              }
              /* interpretation done */
              if (bytebuff == '\n') nline += 1;
            } else {
              value[curval] = bytebuff;
              curval += 1;
              if ((curval + 1) >= maxval) {
                parserstate = 9; /* ignore the line */
                kitten_printf(7, 9, "Warning: Config file value overflow on line #%d", nline);
                puts("");
              }
          }
          break;
        case 9: /* Got comment, ignoring the rest of line */
          if (bytebuff == EOF) break;
          if (bytebuff == '\n') {
            nline += 1;
            parserstate = 0;
          }
          break;
      }
    }
  } while (bytebuff != EOF);
  fclose(fd);

  /* perform some validations */
  if (checkfordoubledirlist(*dirlist) != 0) return(-1);
  if (validatedirlist(*dirlist) != 0) return(-1);

  return(0);
}
/* This file provides functions for parsing commands and their arguments

   Warning: parsecmd() will modify the cmdline string, so it won't be
   readable anymore in any other way other than via ptrtable[].
   This function returns the number of arguments that have been parsed,
   or -1 on parsing error.

   Copyright (C) 2012-2016 Mateusz Viste */

#ifndef PARSECMD_H_SENTINEL
#define PARSECMD_H_SENTINEL

int parsecmd(char *cmdline, char **ptrtable, int maxargs);

#endif

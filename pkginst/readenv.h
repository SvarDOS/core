/*
 * This file is part of pkginst (SvarDOS).
 *
 * Reads environment variables that will be used by pkginst.
 * Returns 0 on success, non-zero otherwise.
 *
 * Copyright (C) 2012-2021 Mateusz Viste
 */

#ifndef READENV_H_SENTINEL
#define READENV_H_SENTINEL

int readenv(char **dosdir, char *cfgfile, int cfgfilemaxlen);

#endif

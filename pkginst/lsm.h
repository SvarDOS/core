/*
 * This file is part of pkg (SvarDOS)
 * Copyright (C) 2013-2021 Mateusz Viste
 */

#ifndef readlsm_h_sentinel
  #define readlsm_h_sentinel
  /* Loads metadata from an LSM file. Returns 0 on success, non-zero on error. */
  int readlsm(const char *filename, char *version, size_t version_maxlen);
#endif

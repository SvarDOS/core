/*
 * This file is part of FDNPKG
 * Copyright (C) 2013-2016 Mateusz Viste, All rights reserved.
 */

#ifndef readlsm_h_sentinel
  #define readlsm_h_sentinel
  /* Loads metadata from an LSM file. Returns 0 on success, non-zero on error. */
  int readlsm(const char *filename, char *version, int version_maxlen);
#endif

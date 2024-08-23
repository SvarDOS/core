/*
 * This file is part of pkg (SvarDOS)
 * Copyright (C) 2013-2024 Mateusz Viste
 */

#ifndef readlsm_h_sentinel
  #define readlsm_h_sentinel
  /* Loads metadata from an LSM file. Returns 0 on success, non-zero on error. */
  int readlsm(const char *filename, const char *field, char *result, size_t result_maxlen);
#endif

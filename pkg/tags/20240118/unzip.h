/*
 * simple unzip tool that unzips the content of a zip archive to current directory
 * returns 0 on success
 *
 * this file is part of pkg (SvarDOS)
 * copyright (C) 2021 Mateusz Viste
 */

#ifndef unzip_h
#define unzip_h

int unzip(const char *zipfile);

#endif

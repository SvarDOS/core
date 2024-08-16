/*
 * simple unzip tool that unzips the content of a zip archive to current directory
 * if listonly is set to a non-zero value then unzip() only lists the files
 * returns 0 on success
 *
 * this file is part of pkg (SvarDOS)
 * copyright (C) 2021-2024 Mateusz Viste
 */

#ifndef unzip_h
#define unzip_h

int unzip(const char *zipfile, unsigned char listonly);

#endif

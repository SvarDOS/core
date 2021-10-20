#ifndef HELPERS_H
#define HELPERS_H

/* case-insensitive comparison of strings, returns non-zero on equality */
int imatch(const char *s1, const char *s2);

/* returns zero if s1 starts with s2 */
int strstartswith(const char *s1, const char *s2);

#endif

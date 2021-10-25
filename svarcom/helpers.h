#ifndef HELPERS_H
#define HELPERS_H

/* case-insensitive comparison of strings, returns non-zero on equality */
int imatch(const char *s1, const char *s2);

/* returns zero if s1 starts with s2 */
int strstartswith(const char *s1, const char *s2);

/* outputs a NULL-terminated string to stdout */
void output_internal(const char *s, unsigned short nl);

#define output(x) output_internal(x, 0)
#define outputnl(x) output_internal(x, 1)

#endif

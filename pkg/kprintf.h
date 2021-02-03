/*
 * The following is a macro-like function that uses kitten instead
 * of directly doing printf.
 */

#ifndef kprintf_sentinel
#define kprintf_sentinel

void kitten_printf(short x, short y, char *fmt, ...);
void kitten_puts(short x, short y, char *fmt);

#endif

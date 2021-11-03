#ifndef REDIR_H
#define REDIR_H

/* parse commandline and performs necessary redirections. cmdline is
 * modified so all redirections are cut out.
 * returns 0 on success, non-zero otherwise */
int redir_parsecmd(char far *cmdline, char *BUFFER);

/* restores previous stdout/stdin handlers if they have been redirected */
void redir_revert(void);

#endif

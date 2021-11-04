#ifndef CMD_H
#define CMD_H

/* process internal commands */
int cmd_process(unsigned short env_rmod, unsigned short env_seg, const char far *cmdline, char *BUFFER);

/* explodes a command into an array of arguments where last arg is NULL
 * returns number of args */
unsigned short cmd_explode(char *buff, const char far *s, char const **argvlist);

#endif

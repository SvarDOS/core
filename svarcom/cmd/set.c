/*
 *
 */

static int cmd_set(int argc, char const **argv, unsigned short env_seg) {
  char far *env = MK_FP(env_seg, 0);
  char buff[256];
  int i;
  while (*env != 0) {
    /* copy string to local buff for display */
    for (i = 0;; i++) {
      buff[i] = *env;
      env++;
      if (buff[i] == 0) break;
    }
    puts(buff);
  }

  return(0);
}

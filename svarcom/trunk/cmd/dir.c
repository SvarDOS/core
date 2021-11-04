/*
 * dir
 *
 * Displays a list of files and subdirectories in a directory.
 *
 * DIR [drive:][path][filename] [/P] [/W] [/A[:]attributes] [/O[[:]sortorder]] [/S] [/B] [/L]
 *
 * /P Pauses after each screenful of information.
 * /W Uses wide list format.
 *
 * /A Displays file with specified attributes:
 *     D Directories           R Read-only files     H Hidden files
 *     A Ready for archiving   S System files        - prefix meaning "not"
 *
 * /O List files in sorted order:
 *     N by name            S by size              E by extension
 *     D by date            G group dirs first     - prefix to reverse order
 *
 * /S Displays files in specified directory and all subdirectories.
 * /B Uses bare format (no heading information or summary)
 * /L Uses lowercases
 */

/* NOTE: /A attributes are matched in an exclusive way, ie. only files with
 *       the specified attributes are matched. This is different from how DOS
 *       itself matches attributes hence DIR cannot rely on the attributes
 *       filter within FindFirst.
 *
 * NOTE: Multiple /A are not supported - only the last one is significant.
 */

static int cmd_dir(struct cmd_funcparam *p) {
  const char *filespecptr = NULL;
  struct DTA *dta = (void *)0x80; /* set DTA to its default location at 80h in PSP */
  int i;
  unsigned short availrows;  /* counter of available rows on display (used for /P) */
  #define DIR_FLAG_PAUSE  1
  #define DIR_FLAG_WIDE   2
  #define DIR_FLAG_RECUR  4
  #define DIR_FLAG_BARE   8
  #define DIR_FLAG_LCASE 16
  unsigned char flags = 0;
//  unsigned char attribs_show = 0; /* show only files with ALL these attribs */
//  unsigned char attribs_hide = 0; /* hide files with ANY of these attribs */

  if (cmd_ishlp(p)) {
    outputnl("Displays a list of files and subdirectories in a directory");
    outputnl("");
    outputnl("DIR [drive:][path][filename] [/P] [/W] [/A[:]attributes] [/O[[:]sortorder]] [/S] [/B] [/L]");
    outputnl("");
    outputnl("/P Pauses after each screenful of information");
    outputnl("/W Uses wide list format");
    outputnl("");
    outputnl("/A Displays files with specified attributes:");
    outputnl("    D Directories            R Read-only files        H Hidden files");
    outputnl("    A Ready for archiving    S System files           - prefix meaning \"not\"");
    outputnl("");
    outputnl("/O List files in sorted order:");
    outputnl("    N by name                S by size                E by extension");
    outputnl("    D by date                G group dirs first       - prefix to reverse order");
    outputnl("");
    outputnl("/S Displays files in specified directory and all subdirectories");
    outputnl("/B Uses bare format (no heading information or summary)");
    outputnl("/L Uses lowercases");

    /* TODO FIXME REMOVE THIS ONCE ALL IMPLEMENTED */
    outputnl("\r\n*** THIS COMMAND IS NOT FULLY IMPLEMENTED YET ***");

    return(-1);
  }

  /* parse command line */
  for (i = 0; i < p->argc; i++) {
    if (p->argv[i][0] == '/') {
      char arg;
      char neg = 0;
      /* detect negations and get actual argument */
      if (p->argv[i][1] == '-') neg = 1;
      arg = p->argv[i][1 + neg];
      /* */
      switch (arg) {
        case 'a':
        case 'A':
          /* TODO */
          outputnl("/A NOT IMPLEMENTED YET");
          return(-1);
          break;
        case 'b':
        case 'B':
          flags |= DIR_FLAG_BARE;
          break;
        case 'p':
        case 'P':
          flags |= DIR_FLAG_PAUSE;
          if (neg) flags &= (0xff ^ DIR_FLAG_PAUSE);
          break;
        default:
          outputnl("Invalid switch");
          return(-1);
      }
    } else {  /* filespec */
      if (filespecptr != NULL) {
        outputnl("Too many parameters");
        return(-1);
      }
      filespecptr = p->argv[i];
    }
  }

  if (filespecptr == NULL) filespecptr = ".";

  {
    unsigned short r = file_truename(filespecptr, p->BUFFER);
    if (r != 0) {
      outputnl(doserr(r));
      return(-1);
    }
  }

  if ((flags & DIR_FLAG_BARE) == 0) {
    unsigned char drv = p->BUFFER[0];
    char *buff2 = p->BUFFER + (BUFFER_SIZE / 2);
    if (drv >= 'a') {
      drv -= 'a';
    } else {
      drv -= 'A';
    }
    cmd_vol_internal(drv, buff2);
    sprintf(buff2, "Directory of %s", p->BUFFER);
    /* trim at first '?', if any */
    for (i = 0; buff2[i] != 0; i++) if (buff2[i] == '?') buff2[i] = 0;
    outputnl(buff2);
    outputnl("");
  }

  /* if dir then append \????????.??? */
  i = file_getattr(p->BUFFER);
  if ((i > 0) && (i & DOS_ATTR_DIR)) strcat(p->BUFFER, "\\????????.???");

  if (findfirst(dta, p->BUFFER, DOS_ATTR_RO | DOS_ATTR_HID | DOS_ATTR_SYS | DOS_ATTR_DIR | DOS_ATTR_ARC) != 0) return(-1);

  availrows = screen_getheight();

  outputnl(dta->fname);
  availrows--;

  while (findnext(dta) == 0) {
    outputnl(dta->fname);
    if (flags & DIR_FLAG_PAUSE) {
      availrows--;
      if (availrows < 2) {
        press_any_key();
        availrows = screen_getheight();
      }
    }
  }

  return(-1);
}

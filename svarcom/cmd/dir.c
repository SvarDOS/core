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

static int cmd_dir(struct cmd_funcparam *p) {
  const char *filespecptr = "*.*";
  struct DTA *dta = (void *)0x80; /* set DTA to its default location at 80h in PSP */

  if (findfirst(dta, filespecptr, DOS_ATTR_RO | DOS_ATTR_HID | DOS_ATTR_SYS | DOS_ATTR_DIR | DOS_ATTR_ARC) != 0) return(-1);

  outputnl(dta->fname);

  while (findnext(dta) == 0) outputnl(dta->fname);

  return(-1);
}

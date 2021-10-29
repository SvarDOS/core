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

#define cmd_dir_attr_ro   1
#define cmd_dir_attr_hid  2
#define cmd_dir_attr_sys  4
#define cmd_dir_attr_vol  8
#define cmd_dir_attr_dir 16
#define cmd_dir_attr_arc 32


static int cmd_dir(struct cmd_funcparam *p) {
  const char *filespecptr = "*.*";
  struct DTA *dta = (void *)0x80; /* set DTA to its default location at 80h in PSP */

  if (findfirst(dta, filespecptr, cmd_dir_attr_ro | cmd_dir_attr_hid | cmd_dir_attr_sys | cmd_dir_attr_dir | cmd_dir_attr_arc) != 0) return(-1);

  outputnl(dta->fname);

  while (findnext(dta) == 0) outputnl(dta->fname);

  return(-1);
}

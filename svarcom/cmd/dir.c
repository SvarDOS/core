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


static int cmd_dir(const struct cmd_funcparam *p) {
  const char *filespecptr = "*.*";
  unsigned short errcode;
  _Packed struct DTA {
    char reserved[21];
    unsigned char attr;
    unsigned short time;
    unsigned short date;
    unsigned long size;
    char fname[13];
  } *dta = (void *)0x80; /* set DTA to its default location at 80h in PSP */
/*
 * FileInfoRec (DTA) format:
 * offset size desc
 *    +0   21  reserved
 *  +15h    1  file attr (1=RO 2=Hidden 4=System 8=VOL 16=DIR 32=Archive
 *  +16h    2  time: bits 0-4=bi-seconds (0-30), bits 5-10=minutes (0-59), bits 11-15=hour (0-23)
 *  +18h    2  date: bits 0-4=day(0-31), bits 5-8=month (1-12), bits 9-15=years since 1980
 *  +1ah    4  DWORD file size, in bytes
 *  +1eh   13  13-bytes max ASCIIZ filename
 */

  errcode = 0;
  _asm {
    push ax
    push cx
    push dx
    /* query DTA location */
    /* mov ah, 0x2f */
    /* int 0x21 */ /* DTA address is in ES:BX now */
    /* mov dta+2, es */
    /* mov dta, bx */
    /* set DTA location */
    mov ah, 0x1a
    mov dx, dta
    int 0x21
    /* FindFirst */
    mov ah, 0x4e
    mov dx, filespecptr   /* filespec */
    mov cx, 0xff         /* file attr to match: when a bit is clear, files with that attr will NOT be found */
    int 0x21
    jnc DONE
    mov errcode, ax
    DONE:
    pop dx
    pop cx
    pop ax
  }
  if (errcode != 0) return(-1);

  outputnl(dta->fname);

  NEXTFILE:

  _asm {
    push ax
    push cx
    push dx
    mov ah, 0x4f /* FindNext */
    mov dx, dta
    int 0x21
    jnc DONE
    mov errcode, ax
    DONE:
    pop dx
    pop cx
    pop ax
  }

  outputnl(dta->fname);

  if (errcode == 0) goto NEXTFILE;

  return(-1);
}

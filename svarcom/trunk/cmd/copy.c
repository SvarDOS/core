/*
 * copy
 */

/* /A - Used to copy ASCII files. Applies to the filename preceding it and to
 * all following filenames. Files will be copied until an end-of-file mark is
 * encountered in the file being copied. If an end-of-file mark is encountered
 * in the file, the rest of the file is not copied. DOS will append an EOF
 * mark at the end of the copied file.
 *
 * /B - Used to copy binary files. Applies to the filename preceding it and to
 * all following filenames. Copied files will be read by size (according to
 * the number of bytes indicated in the file`s directory listing). An EOF mark
 * is not placed at the end of the copied file.
 *
 * /V - Checks after the copy to assure that a file was copied correctly. If
 * the copy cannot be verified, the program will display an error message.
 * Using this option will result in a slower copying process.
 */

struct copy_setup {
  const char *src[64];
  unsigned short src_count; /* how many sources are declared */
  const char *dst;
  char src_asciimode[64];
  char dst_asciimode;
  char last_asciimode; /* /A or /B impacts the file preceding it and becomes the new default for all files that follow */
  char verifyflag;
  char lastitemwasplus;
  char databuf[BUFFER_SIZE - 512];
};

static int cmd_copy(struct cmd_funcparam *p) {
  struct copy_setup *setup = (void *)(p->BUFFER);
  unsigned short i;

  if (cmd_ishlp(p)) {
    outputnl("Copies one or more files to another location.");
    outputnl("");
    outputnl("COPY [/A|/B] source [/A|/B] [+source [/A|/B] [+...]] [destination [/A|/B]] [/V]");
    outputnl("");
    outputnl("source       Specifies the file or files to be copied");
    outputnl("/A           Indicates an ASCII text file");
    outputnl("/B           Indicates a binary file");
    outputnl("destination  Specifies the directory and/or filename for the new file(s)");
    outputnl("/V           Verifies that new files are written correctly");
    outputnl("");
    outputnl("To append files, specify a single file for destination, but multiple files");
    outputnl("for source (using wildcards or file1+file2+file3 format).");
    return(-1);
  }

  /* parse cmdline and fill the setup struct accordingly */

  memset(setup, 0, sizeof(*setup));

  for (i = 0; i < p->argc; i++) {

    /* switch? */
    if (p->argv[i][0] == '/') {
      if ((imatch(p->argv[i], "/a")) || (imatch(p->argv[i], "/b"))) {
        setup->last_asciimode = 'b';
        if (imatch(p->argv[i], "/a")) setup->last_asciimode = 'a';
        /* */
        if (setup->dst != NULL) {
          setup->dst_asciimode = setup->last_asciimode;
        } else if (setup->src_count != 0) {
          setup->src_asciimode[setup->src_count - 1] = setup->last_asciimode;
        }
      } else if (imatch(p->argv[i], "/v")) {
        setup->verifyflag = 1;
      } else {
        outputnl("Invalid switch");
        return(-1);
      }
      continue;
    }

    /* not a switch - must be either a source, a destination or a + */
    if (p->argv[i][0] == '+') {
      /* a plus cannot appear after destination or before first source */
      if ((setup->dst != NULL) || (setup->src_count == 0)) {
        outputnl("Invalid syntax");
        return(-1);
      }
      setup->lastitemwasplus = 1;
      /* a plus may be immediately followed by a filename - if so, emulate
       * a new argument */
      if (p->argv[i][1] != 0) {
        p->argv[i] += 1;
        i--;
      }
      continue;
    }

    /* src? (first non-switch or something that follows a +) */
    if ((setup->lastitemwasplus) || (setup->src_count == 0)) {
      setup->src[setup->src_count] = p->argv[i];
      setup->src_asciimode[setup->src_count] = setup->last_asciimode;
      setup->src_count++;
      setup->lastitemwasplus = 0;
      continue;
    }

    /* must be a dst then */
    if (setup->dst != NULL) {
      outputnl("Invalid syntax");
      return(-1);
    }
    setup->dst = p->argv[i];
    setup->dst_asciimode = setup->last_asciimode;
  }

  /* DEBUG: output setup content ("if 1" to enable) */
  #if 1
  printf("src: ");
  for (i = 0; i < setup->src_count; i++) {
    if (i != 0) printf(", ");
    printf("%s [%c]", setup->src[i], setup->src_asciimode[i]);
  }
  printf("\r\n");
  printf("dst: %s [%c]\r\n", setup->dst, setup->dst_asciimode);
  printf("verify: %s\r\n", (setup->verifyflag)?"ON":"OFF");
  #endif

  /* TODO perform the operation based on setup directives */

  return(-1);
}

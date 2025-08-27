This is a crude Linux (bash) script that converts a directory of text files to
a compressed AMB file. It is mostly useful to convert documentation of the CORE
packages into AMB.

The script performs several tasks:

- generate an index file that lists all the TXT files available (with links)
- word-wrap the TXT files to 78 characters
- escape characters that need to be escaped (%)
- compress the files with MVCOMP
- pack it all up with AMBPACK
- convert files from CR/LF endings to LF-only (more compact)
- convert TABs to 8x spaces
- trim whitespaces at the end of lines

It relies on the following tools:

ambpack, bash, dos2unix, expand, fold, mvcomp, sed


SvarDOS TREE

Run tree with /? for help in using tree.

Usage: TREE [/A] [/F] [path]
/A  - use ASCII (7bit) characters for graphics (lines)
/F  - show files
path indicates which directory to start display from, default is current.

Note: The following nonstandard options are not documented
      with the /? switch.
/V  - show version information,
/S  - force ShortFileNames (SFNs, disable LFN support)
/P  - pause after each page
/DF - display filesizes
/DA - display attributes (Hidden,System,Readonly,Archive)
/DH - display hidden and system files (normally not shown)


=== ORIGINS ===

This program was originally written as pdTREE by Kenneth J. Davis who
maintained it in years 2000-2005.

This version has been forked from pdTREE v1.04 in 2024 by Mateusz Viste who
made a number of changes to simplify the program, port it to Open Watcom,
strip it from non-DOS features, replace all GPL parts, make if faster, lighter
and more memory efficient.


=== LICENSE ===

Copyright (c): Public Domain [United States Definition]
[Note: uses MIT SvarLANG]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR AUTHORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

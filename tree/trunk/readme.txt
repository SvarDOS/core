pdTree v1.04
It is released US public domain.  [note: may use LGPL Cats]
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
/DS - display alternate data streams when exist (Win32 only)
/U  - use Unicode (UTF-8) characters [Experimental, see usage notes in tree.htm]

TODO:
/On - sort* output, where n is F (filesize), N (name), E (extension)
* Output is sorted provided enough memory is available, otherwise output is
  unsorted to avoid aborting due to out of memory errors.

This version is written with the following goals:
  Internationalization, at least support for messages in other languages.
  Long File Name support if available.
  Nearly identical to Win32 version of tree [NT 4 as basis, 98 lacks tree].
  Support for directories with more than 32000 entries. [Yes I've seen this]
  Provide compatibility with FreeDOS tree, so compatible replacement (/Dx options).
  Provide useful enhancements, built in pause (/P), file details (/Dx options),
  sorted* output, and GUI usable output (/U).


This version has a set of hard coded messages.  The messages are
  separated from the code, so they may easily be changed and tree recompiled.
  Additionally, optional support for catgets is included, for easier
  changes without recompiling (except to indicate use of catgets).
  To use the cats message catalogs, the NLSPATH and LANG environment variables
  should be set.  NLSPATH should point to the directory containing the
  message catalogs (files) and LANG should be set to the proper language
  abbreviation.  E.g set NLSPATH=.  set LANG=EN  will result in tree.en
  being using if it is in the current directory, followed by set LANG=ES
  will result in tree.es (the Spanish version) being used.
  When compiled under Windows or DOS with catgets it uses the LGPL
  (refer to LGPL.txt) cats library written by Jim Hall, cats38s.zip.
    Cats, Copyright (C) 1999,2000 Jim Hall <jhall@freedos.org>
    FreeDOS internationalization library
    An implementation of UNIX catgets( ) for DOS
  Note: The necessary files from cats38s.zip are included along with
        the license file, however the full archive should also be
        available at the same location this archive was downloaded from.
        See files.lst for a list of which files are part of Jim Hall's
        cats implementation and thus not public domain.  
  Version 3.8 (unmodified) is the current version of Jim Hall's cats used.

Since I wanted lfn support, I choose the win32 versions of
  findfirst, findnext, findclose as the basis for my routines
  that implement support for lfns.  This allowed me to write tree
  first under windows and test for compatibility with the win32 tree.
  This version should compile under Windows (copyright Microsoft) console mode
  using only the tree.cpp and stack.c files.  To compile under DOS, a 
  second file w32fDOS.cpp is needed, which implements the
  findfirst/next/close routines and any extra routines.

This version (win32 tree.exe and DOS tree.exe [stub]) as best as I
  can tell produces nearly identical output to the WinNT 4 tree program 
  except:
  The volume only includes the 0000:0000 serial number, as I do not know
  where NT's tree is getting the other 8digit number from. 00000000 0000:0000
  (other than it is machine and not volume specific).
  And my program makes no attempt to display the expanded filepath,
  ie changing D:. to D:\somedir\somesubdir
  The error level matches that of the [IBM's] PC DOS 7r1 tree.exe;
  This implementation sets the error level (return value) consistant 
  with the tree program distributed with (copyright) IBM PC DOS 7R1, 
  i.e. 0 on successful display and 1 on most errors (critical errors 
  such as ctrl-C return unspecified values) and if an option is given 
  on the command line (outside of a path to begin at).  Additionally 
  the text displayed on error may show different paths (NT2000pro 
  displays full current path + bad path, followed by error with full 
  path, whereas this implementation shows error with bad path unmodified).
  The only differences I have seen between this version and the WinNT 5
  (aka Windows 2000 Pro.) version are the same as with the WinNT 4 tree,
  and WinNT 5 refers to directories as folders. (This can be changed,
  by editing the tree catalog file if really desired.)

It may be somewhat slower than other implementations, because it must
  open a directory twice, once to get a count of subdirectories and
  then again to actually display information.  The only known
  limitations are on filename length (limited by OS mostly), and the
  maximum depth limited by available memory (a linked list is used to
  store subdirectory information).  Other than slower, it should work 
  (though not tested) with directories with more than 32000 files. 
  (Note w98 explorer will die if you try to 'select all' on that many 
  files. :)

pdTree v1.04
Original location: http://www.darklogic.org/fdos/tree/

  Written by: Kenneth J. Davis
  Date:       August, 2000
  Updated:    September, 2000; October, 2000; November, 2000; January, 2001;
              May, 2004; Sept, 2005
  Contact:    jeremyd@computer.org


Copyright (c): Public Domain [United States Definition]
[Note: may use LGPL cats by Jim Hall jhall@freedos.org]

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

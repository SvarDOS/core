
buildidx is a little program that generates index.lst files for FDNPKG repositories out of a directory filled with FreeDOS packages.

You DON'T need buildidx if you're only using FDNPKG. This is a tool that can be useful only to administrators hosting and maintaining FDNPKG repositories. I tested (and used) it only on a Linux platform, but it might work on other platforms as well (if you manage to compile it correctly).


* Requirements *

To operate properly, buildidx needs following tools to be available:
 - gzip
 - unzip


* How to use it *

Put all your packages into a single directory, for ex. "mystuff", and call buildidx with the directory as a parameter:

./buildidx mystuff

Then buildidx will compute a index.lst file and store it inside your directory. The directory will be useable as a FDNPKG repository then.


- Mateusz Viste

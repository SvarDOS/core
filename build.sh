#!/bin/bash
#
# SvarDOS build script
# http://svardos.osdn.io
# Copyright (C) 2016-2021 Mateusz Viste
#
# This script generates the SvarDOS repository index and builds ISO CD images.
# It should be executed each time that a package has been modified, added or
# removed.
#

### parameters block starts here ############################################

PKGDIR=`realpath ./packages`
REPOROOT=`realpath ./website/repo`
BUILDIDX=`realpath ./buildidx/buildidx`
PUBDIR=`realpath ./website/download`
CDROOT=`realpath ./cdroot`
CUSTFILES=`realpath ./files`

GENISOIMAGE=''    # can be mkisofs, genisoimage or empty for autodetection

### parameters block ends here ##############################################

# auto-detect whether to use mkisofs or genisoimage

if [ "x$GENISOIMAGE" == "x" ] ; then
mkisofs --help 2> /dev/null
if [ $? -eq 0 ] ; then
  GENISOIMAGE='mkisofs'
fi
fi

if [ "x$GENISOIMAGE" == "x" ] ; then
genisoimage --help 2> /dev/null
if [ $? -eq 0 ] ; then
  GENISOIMAGE='genisoimage'
fi
fi

if [ "x$GENISOIMAGE" == "x" ] ; then
  echo "ERROR: neither genisoimage nor mkisofs was found on this system"
  exit 1
fi


# abort if anything fails
set -e


# function that builds the packages repository
function dorepo {
  # copy all zip files to the web repo
  cp "$PKGDIR"/* $REPOROOT/
  # now strip the sources from repo versions
  find "$REPOROOT/" -iname '*.zip' -exec zip "{}" -d "SOURCE/*" ';'
  find "$REPOROOT/" -iname '*.zip' -exec zip "{}" -d "source/*" ';'
  find "$REPOROOT/" -iname '*.zip' -exec zip "{}" -d "Source/*" ';'

  # build repo idx
  $BUILDIDX "$REPOROOT/"
}

### actual code flow starts here ############################################

# check presence of the buildidx tool
if [ ! -f "$BUILDIDX" ] ; then
  echo "buildidx not found at $BUILDIDX"
  exit 1
fi

# remember where I am, so I can get back here once all is done
origdir=`pwd`

# build the boot (install) floppy image first
cp $CUSTFILES/bootmini.img $CDROOT/boot.img
export MTOOLS_NO_VFAT=1
mcopy -sQm -i "$CDROOT/boot.img" $CUSTFILES/floppy/* ::/
if [ $? -ne 0 ] ; then exit 1 ; fi

# build the repo (also builds the listing.txt file)
dorepo

# delete previous (if any) *.iso and *.md5 files
echo "cleaning up old versions..."
rm -f "$PUBDIR/svardos.iso" "$PUBDIR/svardos.iso.md5"

CDISO="$PUBDIR/svardos.iso"

$GENISOIMAGE -input-charset cp437 -b boot.img -iso-level 1 -f -V SVARDOS -o "$CDISO" "$CDROOT"
if [ $? -ne 0 ] ; then exit 1 ; fi

# compute the MD5 of the ISO file, taking care to include only the filename in it
echo "computing md5 sums..."
cd `dirname "$CDISO"`
md5sum `basename "$CDISO"` > "$CDISO.md5"
if [ $? -ne 0 ] ; then exit 1 ; fi

cd "$origdir"

#cd svnlschk
#./webgen.sh
#cd ..

echo "ALL DONE!"

exit 0

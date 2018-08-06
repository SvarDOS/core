#!/bin/bash
#
# Svarog386 build script
# http://svarog386.sourceforge.net
# Copyright (C) 2016-2018 Mateusz Viste
#
# This script generates indexes of Svarog386 repositories and builds the ISO
# CD images. It should be executed each time that a package file have been
# modified, added or removed from any of the repositories.
#

### parameters block starts here ############################################

REPOROOT=`realpath ./website/repos/`
REPOROOTSRC=`realpath ./cdroot/`
REPOROOTNOSRC=`realpath ./cdrootnosrc/`
BUILDIDX=`realpath ../fdnpkg/trunk/buildidx/buildidx`
CDISODIR=`realpath ./iso/`
PUBDIR=`realpath ./website/`
CDROOT=`realpath ./cdroot`
CDROOTNOSRC=`realpath ./cdrootnosrc`
CDROOTMICRO=`realpath ./cdrootmicro`
CUSTFILES=`realpath ./files`

### parameters block ends here ##############################################

# function to be called with the repository (short) name as argument
function dorepo {
  repodir="$1"
  # switch to repo's dir
  cd $REPOROOT/$repodir
  # duplicate all zip files as zib
  for f in *.zip
  do
  cp -p $f `basename -s .zip $f`.zib
  done
  # now strip the sources from the 'zib' versions
  find $REPOROOT/$repodir -iname '*.zib' -exec zip "{}" -d "SOURCE/*" ';'
  find $REPOROOT/$repodir -iname '*.zib' -exec zip "{}" -d "source/*" ';'
  find $REPOROOT/$repodir -iname '*.zib' -exec zip "{}" -d "Source/*" ';'

  # build idx for the full (net) repo
  $BUILDIDX "$REPOROOT/$repodir"
  # copy listing into the global listing
  cat "$REPOROOT/$repodir/listing.txt" >> "$PUBDIR/listing.txt"

  # compute links for the 'no source' version
  mkdir -p $REPOROOTNOSRC/$repodir
  cd $REPOROOTNOSRC/$repodir
  if [ $? -ne 0 ] ; then exit 1 ; fi
  rm *.zip
  for f in $REPOROOT/$repodir/*.zib
  do
  ln -s "$f" `basename -s .zib $f`.zip
  done
  # build idx for the 'no source' version
  $BUILDIDX $REPOROOTNOSRC/$repodir

  # compute links for the 'with source' (but no zib) version
  mkdir -p $REPOROOTSRC/$repodir
  cd $REPOROOTSRC/$repodir
  if [ $? -ne 0 ] ; then exit 1 ; fi
  rm *.zip
  for f in $REPOROOT/$repodir/*.zip
  do
  ln -s "$f" `basename $f`
  done
  # build idx for the 'no source' version
  $BUILDIDX $REPOROOTSRC/$repodir
}

### actual code flow starts here ############################################

# check presence of the buildidx tool
if [ ! -f "$BUILDIDX" ] ; then
  echo "buildidx not found at $BUILDIDX"
  exit 1
fi

# remember where I am, so I can get back here once all is done
origdir=`pwd`

# build the boot floppy image first
cp $CUSTFILES/bootmini.img $CDROOT/boot.img
export MTOOLS_NO_VFAT=1
mcopy -sQm -i "$CDROOT/boot.img" $CUSTFILES/floppy/* ::/
if [ $? -ne 0 ] ; then exit 1 ; fi

# sync the boot.img file from full version to nosrc and micro, and publish it also stand-alone
cp "$CDROOT/boot.img" "$CDROOTNOSRC/"
if [ $? -ne 0 ] ; then exit 1 ; fi
cp "$CDROOT/boot.img" "$CDROOTMICRO/"
if [ $? -ne 0 ] ; then exit 1 ; fi
cp "$CDROOT/boot.img" "$PUBDIR/"
if [ $? -ne 0 ] ; then exit 1 ; fi

# remove the 'human' listing (will be regenerated in a short moment)
rm "$PUBDIR/listing.txt"

# process all repos (also builds the listing.txt file)
dorepo core
dorepo devel
dorepo drivers
dorepo edit
dorepo emulatrs
dorepo games
dorepo net
dorepo packers
dorepo sound
dorepo util

# delete all (previous) *.iso and *.md5 files
echo "cleaning up old versions..."
rm -r $CDISODIR/*

# compute a filename for the ISO files and build it
DATESTAMP=`date +%Y%m%d-%H%M`
YEAR=`date +%Y`

mkdir -p "$CDISODIR/$YEAR"

CDISO="$CDISODIR/$YEAR/svarog386-$DATESTAMP-full.iso"
CDISONOSRC="$CDISODIR/$YEAR/svarog386-$DATESTAMP-nosrc.iso"
CDISOMICRO="$CDISODIR/$YEAR/svarog386-$DATESTAMP-micro.iso"
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -V SVAROG386 -o "$CDISO" "$CDROOT"
if [ $? -ne 0 ] ; then exit 1 ; fi
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -V SVAROG386 -o "$CDISONOSRC" "$CDROOTNOSRC"
if [ $? -ne 0 ] ; then exit 1 ; fi
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -V SVAROG386 -o "$CDISOMICRO" "$CDROOTMICRO"
if [ $? -ne 0 ] ; then exit 1 ; fi

# compute the MD5 of the ISO files, taking care to include only the filename in it
echo "computing md5 sums..."
cd `dirname "$CDISO"`
md5sum `basename "$CDISO"` > "$CDISO.md5"
if [ $? -ne 0 ] ; then exit 1 ; fi

cd `dirname "$CDISONOSRC"`
md5sum `basename "$CDISONOSRC"` > "$CDISONOSRC.md5"
if [ $? -ne 0 ] ; then exit 1 ; fi

cd `dirname "$CDISOMICRO"`
md5sum `basename "$CDISOMICRO"` > "$CDISOMICRO.md5"
if [ $? -ne 0 ] ; then exit 1 ; fi

# compute the ini file with properties of each ISO
echo "[micro]" > "$PUBDIR/downloads.ini"
echo "url=\"https://sourceforge.net/projects/svarog386/files/$YEAR/svarog386-$DATESTAMP-micro.iso/download\"" >> "$PUBDIR/downloads.ini"
echo "md5=\"https://sourceforge.net/projects/svarog386/files/$YEAR/svarog386-$DATESTAMP-micro.iso.md5/download\"" >> "$PUBDIR/downloads.ini"
echo "size=`stat --format='%s' $CDISOMICRO`" >> "$PUBDIR/downloads.ini"
echo "date=`stat --format='%Y' $CDISOMICRO`" >> "$PUBDIR/downloads.ini"
echo "" >> "$PUBDIR/downloads.ini"
echo "[full]" >> "$PUBDIR/downloads.ini"
echo "url=\"https://sourceforge.net/projects/svarog386/files/$YEAR/svarog386-$DATESTAMP-full.iso/download\"" >> "$PUBDIR/downloads.ini"
echo "md5=\"https://sourceforge.net/projects/svarog386/files/$YEAR/svarog386-$DATESTAMP-full.iso.md5/download\"" >> "$PUBDIR/downloads.ini"
echo "size=`stat --format='%s' $CDISO`" >> "$PUBDIR/downloads.ini"
echo "date=`stat --format='%Y' $CDISO`" >> "$PUBDIR/downloads.ini"
echo "" >> "$PUBDIR/downloads.ini"
echo "[nosrc]" >> "$PUBDIR/downloads.ini"
echo "url=\"https://sourceforge.net/projects/svarog386/files/$YEAR/svarog386-$DATESTAMP-nosrc.iso/download\"" >> "$PUBDIR/downloads.ini"
echo "md5=\"https://sourceforge.net/projects/svarog386/files/$YEAR/svarog386-$DATESTAMP-nosrc.iso.md5/download\"" >> "$PUBDIR/downloads.ini"
echo "size=`stat --format='%s' $CDISONOSRC`" >> "$PUBDIR/downloads.ini"
echo "date=`stat --format='%Y' $CDISONOSRC`" >> "$PUBDIR/downloads.ini"

cd "$origdir"

cd svnlschk
./webgen.sh
cd ..

echo "all done!"

exit 0

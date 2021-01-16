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
# usage: ./build.sh [noclean]
#

### parameters block starts here ############################################

PKGDIR=`realpath ./packages`
REPOROOT=`realpath ./website/repo`
BUILDIDX=`realpath ./buildidx/buildidx`
PUBDIR=`realpath ./website/download`
CDROOT=`realpath ./cdroot`
FLOPROOT=`realpath ./floproot`
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


# list of packages to be part of CORE
COREPKGS="attrib chkdsk choice command cpidos ctmouse deltree devload diskcopy display dosfsck edit fc fdapm fdisk fdnpkg format himemx kernel keyb keyb_lay label mem mode more move shsucdx sort tree undelete xcopy udvd2"



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


# prepares image for floppy sets of size $1 and in number of $2 floppies
function prep_flop {
  mkdir $1
  mformat -C -f $1 -v SVARDOS -B "$CUSTFILES/floppy.mbr" -i "$1/1.img"
  mcopy -sQm -i "$1/1.img" "$FLOPROOT/"* ::/
  for i in $(seq 2 $2) ; do
    mformat -C -f $1 -v SVARDOS -i "$1/$i.img"
  done
}


### actual code flow starts here ############################################

# check presence of the buildidx tool
if [ ! -f "$BUILDIDX" ] ; then
  echo "buildidx not found at $BUILDIDX"
  exit 1
fi

# remember where I am, so I can get back here once all is done
origdir=`pwd`

mkdir "$CDROOT"
mkdir "$FLOPROOT"

# build the repo (also builds the listing.txt file)
dorepo

# add CORE packages to CDROOT + create the list of packages on floppy
for pkg in $COREPKGS ; do
  cp "$REPOROOT/$pkg.zip" "$CDROOT/"
  echo "$pkg" >> "$FLOPROOT/install.lst"
done

# prepare the content of the boot (install) floppy
cp "install/install.com" "$FLOPROOT/"
cp "install/nls/"install.?? "$FLOPROOT/"
cp -r "$CUSTFILES/floppy/"* "$FLOPROOT/"

# build the boot (CD) floppy image
export MTOOLS_NO_VFAT=1
mformat -C -f 1440 -v SVARDOS -B "$CUSTFILES/floppy.mbr" -i "$CDROOT/boot.img"
mcopy -sQm -i "$CDROOT/boot.img" "$FLOPROOT/"* ::/

# prepare images for floppies in different sizes and numbers
prep_flop 1440 3
prep_flop 1200 3
prep_flop 720 4

# now populate the floppies sets
#xxxxxxx

# delete previous (if any) *.iso and *.md5 files
echo "cleaning up old versions..."
rm -f "$PUBDIR/svardos.iso" "$PUBDIR/svardos.iso.md5"

CDISO="$PUBDIR/svardos.iso"

$GENISOIMAGE -input-charset cp437 -b boot.img -iso-level 1 -f -V SVARDOS -o "$CDISO" "$CDROOT"

# cleanup temporary things
if [ "x$1" != "xnoclean" ] ; then
  rm -rf "$CDROOT" "$FLOPROOT"
fi

# compute the MD5 of the ISO file, taking care to include only the filename in it
echo "computing md5 sums..."
cd `dirname "$CDISO"`
md5sum `basename "$CDISO"` > "$CDISO.md5"

cd "$origdir"

#cd svnlschk
#./webgen.sh
#cd ..

echo "ALL DONE!"

exit 0

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


# prepares image for floppy sets of size $1
function prep_flop {
  mkdir $1
  mformat -C -f $1 -v SVARDOS -B "$CUSTFILES/floppy.mbr" -i "$1/disk1.img"
  mcopy -sQm -i "$1/disk1.img" "$FLOPROOT/"* ::/

  # now populate the floppies
  curdisk=1
  for p in $COREPKGS ; do
    # if copy fails, then probably the floppy is full - try again after
    # creating an additional floppy image
    if ! mcopy -mi "$1/disk$curdisk.img" "$CDROOT/$p.zip" ::/ ; then
      curdisk=$((curdisk+1))
      mformat -C -f $1 -v SVARDOS -i "$1/disk$curdisk.img"
      mcopy -mi "$1/disk$curdisk.img" "$CDROOT/$p.zip" ::/
    fi
  done

  # add a short readme
  echo "This directory contains a set of $curdisk floppy images of the SvarDOS distribution in the $1 KB floppy format." > "$1/readme.txt"
  echo "" >> "$1/readme.txt"
  echo "These images are raw floppy disk dumps. To write them on an actual floppy disk, you have to use a low-level sector copying tool, like dd." >> "$1/readme.txt"
  echo "" >> "$1/readme.txt"
  echo "Latest SvarDOS version is available on the project's homepage: http://svardos.osdn.io" >> "$1/readme.txt"

  unix2dos "$1/readme.txt"

  # zip the images (and remove them at the same time)
  rm -f "$PUBDIR/svardos-floppy-$1k.zip"
  zip -9 -rmj "$PUBDIR/svardos-floppy-$1k.zip" $1/*

  # clean up
  rmdir $1
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
prep_flop 2880
prep_flop 1440
prep_flop 1200
prep_flop 720

CDISO="$PUBDIR/svardos-cd.iso"
CDZIP="$PUBDIR/svardos-cd.zip"

# delete previous (if any) iso
echo "cleaning up old versions..."
rm -f "$CDISO" "$CDZIP"

$GENISOIMAGE -input-charset cp437 -b boot.img -iso-level 1 -f -V SVARDOS -o "$CDISO" "$CDROOT"

# compress the ISO
zip -mj9 "$CDZIP" "$CDISO"

# cleanup temporary things
if [ "x$1" != "xnoclean" ] ; then
  rm -rf "$CDROOT" "$FLOPROOT"
fi

cd "$origdir"

#cd svnlschk
#./webgen.sh
#cd ..

echo "ALL DONE!"

exit 0

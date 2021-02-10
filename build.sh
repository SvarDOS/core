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
COREPKGS="amb attrib chkdsk choice command cpidos ctmouse deltree devload diskcopy display dosfsck edit fc fdapm fdisk format himemx kernel keyb keyb_lay label mem mode more move pkg pkgnet shsucdx sort tree undelete xcopy udvd2"



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


# prepares image for floppy sets of:
# $1 cylinders
# $2 heads (sides)
# $3 sectors per track
# $4 size
# $5 where to put a copy of the image (optional)
function prep_flop {
  mkdir $4
  mformat -C -t $1 -h $2 -s $3 -v SVARDOS -B "$CUSTFILES/floppy.mbr" -i "$4/disk1.img"
  mcopy -sQm -i "$4/disk1.img" "$FLOPROOT/"* ::/

  # now populate the floppies
  curdisk=1
  for p in $COREPKGS ; do
    # if copy fails, then probably the floppy is full - try again after
    # creating an additional floppy image
    if ! mcopy -mi "$4/disk$curdisk.img" "$CDROOT/$p.zip" ::/ ; then
      curdisk=$((curdisk+1))
      mformat -C -t $1 -h $2 -s $3 -v SVARDOS -i "$4/disk$curdisk.img"
      mcopy -mi "$4/disk$curdisk.img" "$CDROOT/$p.zip" ::/
    fi
  done

  # add a short readme
  echo "This directory contains a set of $curdisk floppy images of the SvarDOS distribution in the $4 KB floppy format." > "$4/readme.txt"
  echo "" >> "$4/readme.txt"
  echo "These images are raw floppy disk dumps. To write them on an actual floppy disk, you have to use a low-level sector copying tool, like dd." >> "$4/readme.txt"
  echo "" >> "$4/readme.txt"
  echo "Latest SvarDOS version is available on the project's homepage: http://svardos.osdn.io" >> "$4/readme.txt"

  unix2dos "$4/readme.txt"

  # make a copy of the image, if requested
  if [ "x$5" != "x" ] ; then
    cp "$4/disk1.img" $5
  fi

  # zip the images (and remove them at the same time)
  rm -f "$PUBDIR/svardos-floppy-$4k.zip"
  zip -9 -rmj "$PUBDIR/svardos-floppy-$4k.zip" $4/*

  # clean up
  rmdir $4
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
unzip -j packages/pkg.zip bin/pkg.exe -d "$FLOPROOT/"

# build the boot (CD) floppy image
export MTOOLS_NO_VFAT=1
#mformat -C -f 2880 -v SVARDOS -B "$CUSTFILES/floppy.mbr" -i "$CDROOT/boot.img"
#mcopy -sQm -i "$CDROOT/boot.img" "$FLOPROOT/"* ::/

# prepare images for floppies in different sizes (args are C H S SIZE)
prep_flop 80 2 36 2880 "$CDROOT/boot.img"
prep_flop 80 2 18 1440
prep_flop 80 2 15 1200
prep_flop 80 2  9  720

CDISO="$PUBDIR/svardos-cd.iso"
CDZIP="$PUBDIR/svardos-cd.zip"

# delete previous (if any) iso
echo "cleaning up old versions..."
rm -f "$CDISO" "$CDZIP"

$GENISOIMAGE -input-charset cp437 -b boot.img -iso-level 1 -f -V SVARDOS -o "$CDISO" "$CDROOT/boot.img"

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

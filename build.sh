#!/bin/bash
#
# This script generates indexes of Svarog386 repositories and builds the ISO
# CD images. It should be executed each time that a package file have been
# modified, added or removed from any of the repositories.
#

### parameters block starts here ############################################

REPOROOT='/srv/www/svarog386.viste.fr/repos/'
REPOROOTSRC='/srv/www/svarog386.viste.fr/repos-src/'
REPOROOTNOSRC='/srv/www/svarog386.viste.fr/repos-nosrc/'
BUILDIDX='/root/fdnpkg-buildidx/buildidx'
CDISODIR='/srv/www/svarog386.viste.fr/'
CDISODIRTEST='/srv/www/svarog386.viste.fr/test/'
CDROOT='/root/svarog386/cdroot'
CDROOTNOSRC='/root/svarog386/cdrootnosrc'
CDROOTMICRO='/root/svarog386/cdrootmicro'
CUSTFILES='/root/svarog386/files'

### parameters block ends here ##############################################

# function to be called with the repository (short) name as argument
function dorepo {
  repodir=$1
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
  $BUILDIDX $REPOROOT/$repodir
  # copy listing into the global listing
  cat $REPOROOT/$repodir/listing.txt >> $CDISODIR/listing.txt

  # compute links for the 'no source' version
  mkdir -p $REPOROOTNOSRC/$repodir
  cd $REPOROOTNOSRC/$repodir
  if [ $? -ne 0 ] ; then exit 1 ; fi
  rm *.zip *.zib
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
  rm *.zip *.zib
  for f in $REPOROOT/$repodir/*.zip
  do
  ln -s "$f" `basename $f`
  done
  # build idx for the 'no source' version
  $BUILDIDX $REPOROOTSRC/$repodir
}

### actual code flow starts here ############################################


TESTMODE=0
if [ "x$1" = "xprod" ]; then
  TESTMODE=0
elif [ "x$1" = "xtest" ]; then
  TESTMODE=1
  CDISODIR="$CDISODIRTEST"
  mkdir -p "$CDISODIR"
else
  echo "usage: build.sh prod|test"
  exit 1
fi

# remember where we are, so we can return there once all is done
origdir=`pwd`

# build the boot floppy image first
cp $CUSTFILES/bootmini.img $CDROOT/boot.img
export MTOOLS_NO_VFAT=1
mcopy -sQm -i $CDROOT/boot.img $CUSTFILES/floppy/* ::/

# sync the boot.img file from full version to nosrc and micro, and publish it also stand-alone
cp $CDROOT/boot.img $CDROOTNOSRC/
if [ $? -ne 0 ] ; then exit 1 ; fi
cp $CDROOT/boot.img $CDROOTMICRO/
if [ $? -ne 0 ] ; then exit 1 ; fi
cp $CDROOT/boot.img $CDISODIR/
if [ $? -ne 0 ] ; then exit 1 ; fi

# remove the 'human' listing (will be regenerated in a short moment)
rm $CDISODIR/listing.txt

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


# compute a filename for the ISO files and build it
DATESTAMP=`date +%Y%m%d-%H%M`
CDISO="$CDISODIR/svarog386-full-$DATESTAMP.iso"
CDISONOSRC="$CDISODIR/svarog386-nosrc-$DATESTAMP.iso"
CDISOMICRO="$CDISODIR/svarog386-micro-$DATESTAMP.iso"
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -V SVAROG386 -o $CDISO.tmp $CDROOT
if [ $? -ne 0 ] ; then exit 1 ; fi
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -V SVAROG386 -o $CDISONOSRC.tmp $CDROOTNOSRC
if [ $? -ne 0 ] ; then exit 1 ; fi
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -V SVAROG386 -o $CDISOMICRO.tmp $CDROOTMICRO
if [ $? -ne 0 ] ; then exit 1 ; fi
mv $CDISO.tmp $CDISO
mv $CDISONOSRC.tmp $CDISONOSRC
mv $CDISOMICRO.tmp $CDISOMICRO

# compute the MD5 of the ISO files, taking care to include only the filename in it
echo "computing md5 sums..."
cd `dirname $CDISO`
md5sum `basename $CDISO` > $CDISO.md5

cd `dirname $CDISONOSRC`
md5sum `basename $CDISONOSRC` > $CDISONOSRC.md5

cd `dirname $CDISOMICRO`
md5sum `basename $CDISOMICRO` > $CDISOMICRO.md5

# delete all *.iso and *.md5 files, leaving only the 16 most recent
echo "cleaning up old versions..."
ls -tp $CDISODIR/svarog386-*-*.iso* | tail -n +17 | xargs -I {} rm -- {}

cd "$origdir"

echo "all done!"

exit 0

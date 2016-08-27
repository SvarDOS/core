#!/bin/sh
#
# This script generates indexes of Svarog386 repositories and builds the ISO
# CD images. It should be executed each time that a package file have been
# modified, added or removed from any of the repositories.
#

### parameters block starts here ############################################

REPOROOT=/srv/www/svarog386.viste.fr/repos/
BUILDIDX=/root/fdnpkg-buildidx/buildidx
CDISODIR='/srv/www/svarog386.viste.fr/'
CDROOT='/root/svarog386/cdroot'

### parameters block ends here ##############################################

# remember where we are, so we can return there once all is done
origdir=`pwd`

# first go to the root of repositories and refresh'em all
cd $REPOROOT
rm listing.txt
$BUILDIDX base
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX devel
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX drivers
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX edit
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX emulatrs
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX games
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX net
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX packers
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX sound
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX util

# compute a filename for the ISO file and build it
CDISO="$CDISODIR/svarog386-`date +%Y%m%d-%H%M`.iso"
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -o $CDISO.tmp $CDROOT
if [ $? -ne 0 ] ; then exit 1 ; fi
mv $CDISO.tmp $CDISO

# compute the MD5 of the ISO file, taking care to include only the filename in it
cd `dirname $CDISO`
md5sum `basename $CDISO` > $CDISO.md5

cd "$origdir"

exit 0

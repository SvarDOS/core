#!/bin/sh
#
# This script generates indexes of Svarog386 repositories and builds the ISO
# CD images. It should be executed each time that a package file have been
# modified, added or removed from any of the repositories.
#

### parameters block starts here ############################################

REPOROOT=/srv/www/svarog386.viste.fr/repos/
REPOROOTNOSRC=/srv/www/svarog386.viste.fr/repos-nosrc/
BUILDIDX=/root/fdnpkg-buildidx/buildidx
CDISODIR='/srv/www/svarog386.viste.fr/'
CDROOT='/root/svarog386/cdroot'
CDROOTNOSRC='/root/svarog386/cdrootnosrc'
CDROOTMICRO='/root/svarog386/cdrootmicro'

### parameters block ends here ##############################################

# remember where we are, so we can return there once all is done
origdir=`pwd`

# clone the repositories into future 'no source' versions
echo "cloning $REPOROOT to $REPOROOTNOSRC..."
rsync -a --delete $REPOROOT $REPOROOTNOSRC
if [ $? -ne 0 ] ; then exit 1 ; fi

# sync the boot.img file from full version to nosrc and micro
cp $CDROOT/boot.img $CDROOTNOSRC/
if [ $? -ne 0 ] ; then exit 1 ; fi
cp $CDROOT/boot.img $CDROOTMICRO/
if [ $? -ne 0 ] ; then exit 1 ; fi

# now strip the sources from the 'no source' clone
find $REPOROOTNOSRC/ -iname '*.zip' -exec zip "{}" -d "SOURCE/*" ';'
find $REPOROOTNOSRC/ -iname '*.zip' -exec zip "{}" -d "source/*" ';'
find $REPOROOTNOSRC/ -iname '*.zip' -exec zip "{}" -d "Source/*" ';'

# refresh all repositories
$BUILDIDX $REPOROOT/base && $BUILDIDX $REPOROOTNOSRC/base
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/devel && $BUILDIDX $REPOROOTNOSRC/devel
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/drivers && $BUILDIDX $REPOROOTNOSRC/drivers
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/edit && $BUILDIDX $REPOROOTNOSRC/edit
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/emulatrs && $BUILDIDX $REPOROOTNOSRC/emulatrs
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/games && $BUILDIDX $REPOROOTNOSRC/games
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/net && $BUILDIDX $REPOROOTNOSRC/net
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/packers && $BUILDIDX $REPOROOTNOSRC/packers
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/sound && $BUILDIDX $REPOROOTNOSRC/sound
if [ $? -ne 0 ] ; then exit 1 ; fi
$BUILDIDX $REPOROOT/util && $BUILDIDX $REPOROOTNOSRC/util
if [ $? -ne 0 ] ; then exit 1 ; fi

# recompute the listing.txt file
rm $CDISODIR/listing.txt
cat $REPOROOT/base/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/devel/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/drivers/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/edit/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/emulatrs/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/games/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/net/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/packers/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/sound/listing.txt >> $CDISODIR/listing.txt
cat $REPOROOT/util/listing.txt >> $CDISODIR/listing.txt

# compute a filename for the ISO files and build it
DATESTAMP=`date +%Y%m%d-%H%M`
CDISO="$CDISODIR/svarog386-full-$DATESTAMP.iso"
CDISONOSRC="$CDISODIR/svarog386-nosrc-$DATESTAMP.iso"
CDISOMICRO="$CDISODIR/svarog386-micro-$DATESTAMP.iso"
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -o $CDISO.tmp $CDROOT
if [ $? -ne 0 ] ; then exit 1 ; fi
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -o $CDISONOSRC.tmp $CDROOTNOSRC
if [ $? -ne 0 ] ; then exit 1 ; fi
genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -o $CDISOMICRO.tmp $CDROOTMICRO
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

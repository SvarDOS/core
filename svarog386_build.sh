#!/bin/sh

REPOROOT=/srv/www/svarog386.viste.fr/repos/
BUILDIDX=/root/fdnpkg-buildidx/buildidx

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

genisoimage -input-charset cp437 -b boot.img -iso-level 1 -f -o ../svarog386.iso cdroot
cd ..
md5sum svarog386.iso > svarog386.iso.md5

exit 0

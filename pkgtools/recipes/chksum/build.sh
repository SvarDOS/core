#!/bin/sh

NAME=chksum
VERSION="release 2"
CATEGORY=progs
DESCRIPTION="A versatile checksum generator for DOS"

DISTURL="https://cloud.disroot.org/public.php/dav/files/EmNZXrN8oZkr5X2/chksum/release2/dos/?accept=zip"
SRCURL="https://cloud.disroot.org/public.php/dav/files/EmNZXrN8oZkr5X2/chksum/release2/src/chksum-b10d8dceca.zip"

source ../functions.sh

# commands that modify the extracted distribution files under $BUILDDIR
# you can add, modify, delete files that will go into the .SVP package
function adjust_builddir() {
	mv $BUILDDIR/dos/* $BUILDDIR
	rm -rf "$BUILDDIR/dos"
}

run

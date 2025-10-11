#!/bin/sh

NAME=nasm
VERSION=3.01
CATEGORY=devel
HWREQ=386
DESCRIPTION="The Netwide Assembler (UPX-compressed, 386+)"

DISTURL="https://www.nasm.us/pub/nasm/releasebuilds/$VERSION/dos/nasm-$VERSION-dos-upx.zip"
SRCURL="https://www.nasm.us/pub/nasm/releasebuilds/$VERSION/nasm-$VERSION.zip"

source ../functions.sh

# commands that modify the extracted distribution files under $BUILDDIR
# you can add, modify, delete files that will go into the .SVP package
function adjust_builddir() {
	mv $BUILDDIR/nasm-$VERSION/* $BUILDDIR
	rm -rf $BUILDDIR/nasm-$VERSION
}

run

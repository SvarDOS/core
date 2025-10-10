#!/bin/sh

NAME=dojs
VERSION=1.13.0
CATEGORY=devel
DESCRIPTION="A DOS JavaScript Canvas with sound (386+)"
HWREQ=386

DISTURL="https://github.com/SuperIlu/DOjS/releases/download/v1.130/dojs-$VERSION.zip"
#SRCURL="https://github.com/SuperIlu/DOjS/archive/refs/tags/v1.130.zip"
#  src ZIP is ~60MB. Maybe not that good to put it into the SVN

source ../functions.sh

# commands that modify the extracted distribution files under $BUILDDIR
# you can add, modify, delete files that will go into the .SVP package
function adjust_builddir() {
	rm -rf "$BUILDDIR"/doc/*
	rm "$BUILDDIR"/readme_linux.md
	rm "$BUILDDIR"/readme_win32.md
	mv "$BUILDDIR"/changelog.md "$BUILDDIR"/changes.md
	mv "$BUILDDIR"/examples/ntpclient.js "$BUILDDIR"/examples/ntpclnt.js

	(echo "Documentation contains MANY LFNs, so not included here!" | unix2dos) >$BUILDDIR/doc/whereis.txt
	(echo "Please see https://github.com/SuperIlu/DOjS." | unix2dos) >>$BUILDDIR/doc/whereis.txt
}

run

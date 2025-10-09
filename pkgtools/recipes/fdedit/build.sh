#!/bin/sh

NAME=fdedit
VERSION=0.9c
CATEGORY=progs
DESCRIPTION="FreeDOS EDIT, the text editor from the FreeDOS project"

DISTURL="https://github.com/boeckmann/edit/releases/download/v0.9c/edit.zip"
SRCURL="https://github.com/boeckmann/edit/archive/refs/tags/v0.9c.zip"

source ../functions.sh

# commands that modify the extracted distribution files under $BUILDDIR
# you can add, modify, delete files that will go into the .SVP package
function adjust_builddir() {
	mv $BUILDDIR/bin/* $BUILDDIR
	mv $BUILDDIR/doc/* $BUILDDIR
	rm -rf "$BUILDDIR/bin" "$BUILDDIR/doc" "$BUILDDIR/source"
}

run

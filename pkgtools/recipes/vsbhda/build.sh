#!/bin/sh

NAME=vsbhda
VERSION=1.9
CATEGORY=drivers
DESCRIPTION="Sound Blaster emulation for HDA (and AC'97/SB Live!). (386+)"
HWREQ=386

DISTURL="https://github.com/Baron-von-Riedesel/VSBHDA/releases/download/v$VERSION/vsbhda19.zip"
SRCURL="https://github.com/Baron-von-Riedesel/VSBHDA/archive/refs/tags/v$VERSION.zip"

source ../functions.sh

# commands that modify the extracted distribution files under $BUILDDIR
# you can add, modify, delete files that will go into the .SVP package
function adjust_builddir() {
	rm -rf "$BUILDDIR/Win31"
}

run

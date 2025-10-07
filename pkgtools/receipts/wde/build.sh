#!/bin/sh

NAME=wde
VERSION=1.1
CATEGORY=progs
HWREQ=386
DESCRIPTION="Disk editor for all FAT file systems (386+)"
WARN=

DISTURL="https://github.com/Baron-von-Riedesel/WDe/releases/download/v1.1/wde110.zip"
SRCURL="https://github.com/Baron-von-Riedesel/WDe/archive/refs/tags/v1.1.zip"

source ../functions.sh

# commands that modify the extracted distribution files under $BUILDDIR
# you can add, modify, delete files that will go into the .SVP package
function adjust_builddir() {
	true
}

run

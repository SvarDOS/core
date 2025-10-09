#!/bin/sh

NAME=jemm
VERSION=5.85
CATEGORY=drivers
DESCRIPTION='Jemm is an "Expanded Memory Manager". Includes Jemm386, JemmEx. (386+)'
HWREQ=386

DISTURL="https://github.com/Baron-von-Riedesel/Jemm/releases/download/v5.85/JemmB_v585.zip"
SRCURL="https://github.com/Baron-von-Riedesel/Jemm/archive/refs/tags/v5.85.zip"

source ../functions.sh

# commands that modify the extracted distribution files under $BUILDDIR
# you can add, modify, delete files that will go into the .SVP package
function adjust_builddir() {
	true
}

run

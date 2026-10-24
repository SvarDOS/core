#!/bin/sh

NAME=mtcp
VERSION=2026-10-10
CATEGORY=progs
DESCRIPTION="collection of TCP/IP tools for 16 bit DOS: DHCP, IRC, FTP, Telnet, Netcat, HTGet, Ping, SNTP"

DISTURL="https://www.brutman.com/mTCP/download/mTCP_2026-10-10.zip"
SRCURL="https://www.brutman.com/mTCP/download/mTCP-src_2026-10-10.zip"

source ../functions.sh

# commands that modify the extracted distribution files under $BUILDDIR
# you can add, modify, delete files that will go into the .SVP package
function adjust_builddir() {
	true
}

run

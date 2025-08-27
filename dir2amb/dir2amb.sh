#!/bin/bash
#
# converts a directory with text files into an AMB book
# Mateusz Viste, 2025
#
# requirements:
#
# AMBPACK
# BASH
# FOLD
# MVCOMP
# SED
#

set -e

SRCDIR="$1"
TITLE="$2"

if [ "x$TITLE" = "x" ] ; then
echo "This script converts a directory with text files into an AMB book."
echo ""
echo "usage: dir2amb.sh dirname 'title'"
exit 1
fi

DSTDIR="tempdir"


mkdir "$DSTDIR"
cp "$SRCDIR"/* "$DSTDIR"/

# convert all files to AMC (word-wrap + escape chars + compress)
cd "$DSTDIR"

echo "" > ../index.ama
echo "%t$TITLE" >> ../index.ama
echo "" >> ../index.ama

for f in * ; do

# get the filename without extension
FNAME=${f%.*}

echo "* %l$FNAME:$f" >> ../index.ama

# word-wrap at 78 characters and escape (double) every '%'
fold -w 78 -s "$f" | sed 's/%/%%/g' > "$FNAME".ama

# compress
mvcomp "$FNAME".ama "$FNAME".amc
rm "$FNAME".ama
rm "$f"

done

echo "$TITLE" > title

echo "" >> ../index.ama
mvcomp ../index.ama index.amc
rm ../index.ama

cd ..

# pack everything up

ambpack c "$DSTDIR" "$SRCDIR".amb


# clean up
rm "$DSTDIR"/*
rmdir "$DSTDIR"

exit 0

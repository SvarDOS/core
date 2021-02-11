#!/bin/sh
set -e

VER=`date +%Y%m%d`

echo "SVARDOS HELP SYSTEM ver $VER" > help-en/title
ambpack c help-en help-en.amb
rm help-en/title

mkdir bin
mkdir help
mkdir appinfo
cp help.bat bin
mv help-*.amb help
echo "version: $VER" >> appinfo/help.lsm
echo "description: SvarDOS help (manual)" >> appinfo/help.lsm
cp help/help-*.amb ../website/help/
zip -9rkm help.zip appinfo bin help
mv help.zip ../packages/

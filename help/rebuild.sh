#!/bin/sh
set -e

ambpack c help-en help-en.amb
mkdir bin
mkdir help
mkdir appinfo
cp help.bat bin
mv help-*.amb help
echo "version: `date +%Y%m%d`" >> appinfo/help.lsm
echo "description: SvarDOS help (manual)" >> appinfo/help.lsm
cp help/help-*.amb ../website/help/
zip -9rkm help.zip appinfo bin help
mv help.zip ../packages/

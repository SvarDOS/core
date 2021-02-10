#!/bin/sh
set -e

ambpack c help-en help-en.amb
mkdir bin
mkdir help
mkdir appinfo
cp help.bat bin
mv help-*.amb help
echo "version: 20210210" >> appinfo/help.lsm
echo "description: SvarDOS HELP" >> appinfo/help.lsm
zip -9rkm help.zip appinfo bin help

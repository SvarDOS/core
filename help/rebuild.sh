#!/bin/sh
#
# builds AMB-based help files for each available language
# this is a Linux shell script that requires following tools to be in path:
#  - ambpack
#  - zip
#

set -e

VER=`date +%Y%m%d`

# amb-pack all languages
for d in ./help-?? ; do
ambpack cc $d $d.amb
done

mkdir bin
mkdir help
mkdir appinfo
cp help.bat bin
mv help-*.amb help
echo "version: $VER" >> appinfo/help.lsm
echo "description: SvarDOS help (manual)" >> appinfo/help.lsm
cp help/help-*.amb ../website/help/
zip -9rkDX -m help.zip appinfo bin help
rmdir appinfo bin help
mv help.zip ../packages/

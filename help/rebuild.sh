#!/bin/sh
#
# builds AMB-based help files for each available language
# this is a Linux shell script that requires following tools to be in path:
#  - ambpack - http://amb.osdn.io/
#  - utf8tocp - https://utf8tocp.sourceforge.io/
#  - zip
#

set -e

VER=`date +%Y%m%d`

##### amb-pack all languages #####

# EN
echo "SVARDOS HELP SYSTEM ver $VER" > help-en/title
ambpack c help-en help-en.amb

# DE
echo "SVARDOS-HILFESYSTEM ver $VER" > help-de/title
cp -n help-en/* help-de/
utf8tocp -d 858 help-de/unicode.map
ambpack cc help-de help-de.amb
rm help-de/unicode.map

# BR
echo "SISTEMA DE AJUDA DO SVARDOS ver $VER" > help-br/title
cp -n help-en/* help-br/
utf8tocp -d 858 help-br/unicode.map
ambpack cc help-br help-br.amb
rm help-br/unicode.map

mkdir bin
mkdir help
mkdir appinfo
cp help.bat bin
mv help-*.amb help
echo "version: $VER" >> appinfo/help.lsm
echo "description: SvarDOS help (manual)" >> appinfo/help.lsm
zip -9rkDX -m help-$VER.svp appinfo bin help
rmdir appinfo bin help

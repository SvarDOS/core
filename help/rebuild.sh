#!/bin/sh
#
# builds AMB-based help files for each available language
# this is a Linux shell script that requires following tools to be in path:
#  - ambpack  - http://amb.osdn.io/
#  - utf8tocp - https://utf8tocp.sourceforge.io/
#  - zip      - https://infozip.sourceforge.net/
#  - advzip   - http://www.advancemame.it/
#

set -e

VER=`date +%Y%m%d`

##### amb-pack all languages #####

# EN
echo "SVARDOS HELP SYSTEM [$VER]" > help-en/title
ambpack c help-en help-en.amb

# DE
echo "SVARDOS-HILFESYSTEM [$VER]" > help-de/title
utf8tocp -d 858 help-de/unicode.map
ambpack cc help-de help-de.amb
rm help-de/unicode.map

# BR
echo "SISTEMA DE AJUDA DO SVARDOS [$VER]" > help-br/title
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

# repack the ZIP file with advzip for extra space saving
advzip -zp4k -i128 help-$VER.svp

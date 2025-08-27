#!/bin/sh
#
# builds AMB-based help files for each available language
# this is a Linux shell script that requires following tools to be in path:
#  - advzip   - http://www.advancemame.it/
#  - ambpack  - http://mateusz.fr/amb/
#  - mvcomp   - http://mateusz.fr/mvcomp/
#  - utf8tocp - http://mateusz.fr/utf8tocp/
#  - zip      - https://infozip.sourceforge.net/
#

set -e

VER=`date +%Y%m%d`

##### amb-pack all languages #####

function buildamb() {
  folder=$1
  cp=$2
  title=$3

  mkdir tmp

  cp $folder/* tmp
  utf8tocp -d $cp tmp/unicode.map
  echo "$3" > tmp/title.utf8
  utf8tocp $cp tmp/title.utf8 tmp/title
  rm tmp/title.utf8
  for f in tmp/*.ama ; do
    mv $f $f.utf8
    utf8tocp $cp $f.utf8 $f
    mvcomp $f tmp/`basename --suffix .ama $f`.amc
    rm $f $f.utf8
  done
  ambpack c tmp $folder.amb
  rm tmp/*
  rmdir tmp
}


# EN
buildamb help-en 437 "SVARDOS HELP SYSTEM [$VER]"

# DE
buildamb help-de 858 "SVARDOS-HILFESYSTEM [$VER]"

# BR
buildamb help-br 858 "SISTEMA DE AJUDA DO SVARDOS [$VER]"


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

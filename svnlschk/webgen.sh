#!/bin/bash

SVNLSCHK='./svnlschk'
COREDIR='/srv/www/svarog386.viste.fr/repos/core'
HTMLFILE='/srv/www/svarog386.viste.fr/index-nls.htm'

LANGSLIST="en      de     fr     pl     tr"
LANGSLONG="default german french polish turkish"

##############################################################################

processfile() {
  f=$1
  pkgname=$2
  nlstype=$3

  echo "Package: $pkgname"

  if [ $nlstype -eq 0 ] ; then
    LANGS=$LANGSLIST
    ENCOUNT=`$SVNLSCHK $f en $nlstype`
  else
    LANGS=$LANGSLONG
    ENCOUNT=`$SVNLSCHK $f default $nlstype`
  fi

  echo "<tr>" >> $HTMLFILE
  echo "  <td class=\"pkg\">$pkgname</td>" >> $HTMLFILE

  for l in $LANGS
  do

  if [ $ENCOUNT -lt 0 ] ; then
    echo "  <td class=\"err\">ERR</td>" >> $HTMLFILE
  fi

  if [ $ENCOUNT -eq 0 ] ; then
    echo "  <td class=\"unsup\"></td>" >> $HTMLFILE
  fi

  if [ $ENCOUNT -gt 0 ] ; then
    NLSCOUNT=`$SVNLSCHK $f $l $nlstype`
    if [ $ENCOUNT -eq $NLSCOUNT ] ; then
      echo "  <td class=\"complete\">$NLSCOUNT/$ENCOUNT</td>" >> $HTMLFILE
    else
      echo "  <td class=\"incomplete\">$NLSCOUNT/$ENCOUNT</td>" >> $HTMLFILE
    fi
  fi

  done

  echo "</tr>" >> $HTMLFILE
}

##############################################################################

cat head.html > $HTMLFILE

echo "<tr>" >> $HTMLFILE
printf "  <th></th>" >> $HTMLFILE

for l in $LANGSLIST
do
  printf "<th>%s</th>" "$l" >> $HTMLFILE
done

printf "\n</tr>\n" >> $HTMLFILE

# process COMMAND (special NLS format)
processfile $COREDIR/command.zip "command (lng)" 1
processfile $COREDIR/command.zip "command (err)" 2

for f in $COREDIR/*.zip
do

  pkgname=`basename $f .zip`
  # skip COMMAND
  if [ $pkgname = "command" ] ; then
  continue;
  fi

  processfile $f $pkgname 0

done

cat tail.html >> $HTMLFILE

exit 0

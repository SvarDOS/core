#!/bin/bash
#
# SvarDOS build script
# http://svardos.org
# Copyright (C) 2016-2022 Mateusz Viste
#
# This script builds floppy and CD images. It should be executed each time that
# a CORE package has been modified or the build script changed. Before running
# it looks for the presence of a /tmp/svardos_repo_changed.flag and stops if
# no such flag exists. This flag is expected to be created by an svn
# post-commit hook when an important svn change is detected.
#
# usage: ./build.sh outputdir [noclean] > logfile
#

### parameters block starts here ############################################

CURDATE=`date +%Y%m%d`
REPOROOT=`realpath ./packages`
CUSTFILES=`realpath ./files`

GENISOIMAGE=''    # can be mkisofs, genisoimage or empty for autodetection

### parameters block ends here ##############################################

# look for mandatory output dir
if [ "x$1" == "x" ] ; then
  echo "usage: build.sh outputdir [noclean] > logfile"
  exit 1
fi
PUBDIR=`realpath "$1"`/$CURDATE

CDROOT="$PUBDIR/tmp_cdroot.build"
FLOPROOT="$PUBDIR/tmp_floproot.build"


# auto-detect whether to use mkisofs or genisoimage

if [ "x$GENISOIMAGE" == "x" ] ; then
mkisofs --help 2> /dev/null
if [ $? -eq 0 ] ; then
  GENISOIMAGE='mkisofs'
fi
fi

if [ "x$GENISOIMAGE" == "x" ] ; then
genisoimage --help 2> /dev/null
if [ $? -eq 0 ] ; then
  GENISOIMAGE='genisoimage'
fi
fi

if [ "x$GENISOIMAGE" == "x" ] ; then
  echo "ERROR: neither genisoimage nor mkisofs was found on this system"
  exit 1
fi


# abort if anything fails
set -e


# list of packages to be part of CORE (always installed)
COREPKGS=`ls -1 'packages/core' | grep -o '^[a-z]*'`

# list of packages to be part of EXTRA (only sometimes installed, typically drivers)
EXTRAPKGS="pcntpk udvd2"

# all packages
ALLPKGS="$COREPKGS $EXTRAPKGS"


# prepares image for floppy sets of:
# $1 cylinders
# $2 heads (sides)
# $3 sectors per track
# $4 size
# $5 where to put a copy of the image (optional)
function prep_flop {
  mkdir $4
  mformat -C -t $1 -h $2 -s $3 -v SVARDOS -B "$CUSTFILES/floppy.mbr" -i "$4/disk1.img"
  mcopy -sQm -i "$4/disk1.img" "$FLOPROOT/"* ::/

  # now populate the floppies
  curdisk=1
  for p in $ALLPKGS ; do
    # if copy fails, then probably the floppy is full - try again after
    # creating an additional floppy image
    if ! mcopy -mi "$4/disk$curdisk.img" "$CDROOT/$p.svp" ::/ ; then
      curdisk=$((curdisk+1))
      mformat -C -t $1 -h $2 -s $3 -v SVARDOS -i "$4/disk$curdisk.img"
      mcopy -mi "$4/disk$curdisk.img" "$CDROOT/$p.svp" ::/
    fi
  done

  # add a short readme
  echo "This directory contains a set of $curdisk floppy images of the SvarDOS distribution in the $4 KB floppy format." > "$4/readme.txt"
  echo "" >> "$4/readme.txt"
  echo "These images are raw floppy disk dumps. To write them on an actual floppy disk, you have to use a low-level sector copying tool, like dd." >> "$4/readme.txt"
  echo "" >> "$4/readme.txt"
  echo "Latest SvarDOS version is available on the project's homepage: http://svardos.osdn.io" >> "$4/readme.txt"

  unix2dos "$4/readme.txt"

  # make a copy of the image, if requested
  if [ "x$5" != "x" ] ; then
    cp "$4/disk1.img" $5
  fi

  # zip the images (and remove them at the same time)
  rm -f "$PUBDIR/svardos-floppy-$4k.zip"
  zip -9 -rmj "$PUBDIR/svardos-$CURDATE-floppy-$4k.zip" $4/*

  # clean up
  rmdir $4
}


### actual code flow starts here ############################################

# remember where I am, so I can get back here once all is done
origdir=`pwd`

echo "###############################################################################"
echo " STARTING BUILD $CURDATE"
echo "###############################################################################"
echo "dest dir: $PUBDIR"
echo "current time is `date` and it's a beautiful day somewhere in the world"
echo

# remove dest dir if it exists already, then recreate it empty
rm -rf "$PUBDIR"
mkdir "$PUBDIR"

mkdir "$CDROOT"
mkdir "$FLOPROOT"

# add CORE packages to CDROOT + create the list of packages on floppy
for pkg in $COREPKGS ; do
  cp "$REPOROOT/core/$pkg.svp" "$CDROOT/"
  echo "$pkg" >> "$FLOPROOT/install.lst"
done

# add EXTRA packages to CDROOT (but not in the list of packages to install)
for pkg in $EXTRAPKGS ; do
  cp "$REPOROOT/$pkg.svp" "$CDROOT/"
done


echo
echo "### Populating the floppy root at $FLOPROOT"
echo

# prepare the content of the boot (install) floppy
cp -r "$CUSTFILES/floppy/"* "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/cpidos.svp" 'cpi/ega*.cpx' -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/command.svp" bin/command.com -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/display.svp" bin/display.exe -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/edit.svp" bin/edit.exe -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/fdapm.svp" bin/fdapm.com -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/fdisk.svp" bin/fdisk.exe bin/fdiskpt.ini -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/format.svp" bin/format.exe -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/kernel.svp" bin/kernel.sys bin/sys.com -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/mem.svp" bin/mem.exe -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/mode.svp" bin/mode.com -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/more.svp" bin/more.exe -d "$FLOPROOT/"
unzip -Cj "$REPOROOT/core/pkg.svp" bin/pkg.exe -d "$FLOPROOT/"

echo
echo "### Creating floppy images"
echo

# build the boot (CD) floppy image
export MTOOLS_NO_VFAT=1

# prepare images for floppies in different sizes (args are C H S SIZE)
prep_flop 80 2 36 2880 "$CDROOT/boot.img"
prep_flop 80 2 18 1440
prep_flop 80 2 15 1200
prep_flop 80 2  9  720

echo
echo "### Computing DOSEMU.zip"
echo

# prepare the DOSEMU boot zip
DOSEMUDIR="$PUBDIR/tmp_dosemu-prep-files.build"
mkdir "$DOSEMUDIR"
# INSTALL.BAT
echo 'IF NOT EXIST C:\TEMP\NUL MKDIR C:\TEMP' >> "$DOSEMUDIR/install.bat"
echo 'mkdir %DOSDIR%' >> "$DOSEMUDIR/install.bat"
echo 'mkdir %DOSDIR%\cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO # pkg config file - specifies locations where packages should be installed >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO DIR PROGS C:\ >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO DIR GAMES C:\ >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO DIR DRIVERS C:\DRIVERS\ >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO DIR DEVEL C:\DEVEL\ >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
for p in $COREPKGS ; do
  cp "$CDROOT/$p.svp" "$DOSEMUDIR/"
  echo "pkg install $p.svp" >> "$DOSEMUDIR/install.bat"
  echo "del $p.svp" >> "$DOSEMUDIR/install.bat"
done
echo 'ECHO my_ip = dhcp >> %DOSDIR%\CFG\WATTCP.CFG' >> "$DOSEMUDIR/install.bat"
echo 'del pkg.exe' >> "$DOSEMUDIR/install.bat"
echo 'ECHO SHELLHIGH=C:\SVARDOS\BIN\COMMAND.COM /P >> C:\CONFIG.SYS' >> "$DOSEMUDIR/install.bat"
echo 'ECHO.' >> "$DOSEMUDIR/install.bat"
echo 'ECHO -------------------------' >> "$DOSEMUDIR/install.bat"
echo 'ECHO  SVARDOS SETUP COMPLETED ' >> "$DOSEMUDIR/install.bat"
echo 'ECHO -------------------------' >> "$DOSEMUDIR/install.bat"
echo 'ECHO.' >> "$DOSEMUDIR/install.bat"
unzip -Cj "$REPOROOT/core/kernel.svp" bin/kernel.sys -d "$DOSEMUDIR/"
unzip -Cj "$REPOROOT/core/command.svp" bin/command.com -d "$DOSEMUDIR/"
unzip -Cj "$REPOROOT/core/pkg.svp" bin/pkg.exe -d "$DOSEMUDIR/"
# CONFIG.SYS
echo 'FILES=50' >> "$DOSEMUDIR/config.sys"
echo 'DOS=HIGH,UMB' >> "$DOSEMUDIR/config.sys"
echo 'DOSDATA=UMB' >> "$DOSEMUDIR/config.sys"
echo 'DEVICE=D:\dosemu\emufs.sys' >> "$DOSEMUDIR/config.sys"
echo 'DEVICE=D:\dosemu\umb.sys' >> "$DOSEMUDIR/config.sys"
echo 'DEVICEHIGH=D:\dosemu\ems.sys' >> "$DOSEMUDIR/config.sys"
echo 'INSTALL=D:\dosemu\emufs.com' >> "$DOSEMUDIR/config.sys"
# AUTOEXEC.BAT
echo "@ECHO OFF" >> "$DOSEMUDIR/autoexec.bat"
echo 'SET DOSDIR=C:\SVARDOS' >> "$DOSEMUDIR/autoexec.bat"
echo 'SET WATTCP.CFG=%DOSDIR%\CFG' >> "$DOSEMUDIR/autoexec.bat"
echo 'SET DIRCMD=/p/ogne' >> "$DOSEMUDIR/autoexec.bat"
echo 'SET TEMP=C:\TEMP' >> "$DOSEMUDIR/autoexec.bat"
echo 'PATH %DOSDIR%\BIN' >> "$DOSEMUDIR/autoexec.bat"
echo "" >> "$DOSEMUDIR/autoexec.bat"
echo "REM *** this is a one-time setup script used only during first initialization ***" >> "$DOSEMUDIR/autoexec.bat"
echo 'IF EXIST INSTALL.BAT CALL INSTALL.BAT' >> "$DOSEMUDIR/autoexec.bat"
echo 'IF EXIST INSTALL.BAT DEL INSTALL.BAT' >> "$DOSEMUDIR/autoexec.bat"
echo "" >> "$DOSEMUDIR/autoexec.bat"
echo "ECHO." >> "$DOSEMUDIR/autoexec.bat"
echo "ECHO Welcome to SvarDOS (powered by DOSEMU)! Type HELP if you are lost." >> "$DOSEMUDIR/autoexec.bat"
echo "ECHO." >> "$DOSEMUDIR/autoexec.bat"
rm -f "$PUBDIR/svardos-dosemu.zip"
zip -rm9jk "$PUBDIR/svardos-$CURDATE-dosemu.zip" "$DOSEMUDIR"
rmdir "$DOSEMUDIR"

echo
echo "### Computing the USB image"
echo

# prepare the USB bootable image
USBIMG=$PUBDIR/svardos-usb.img
cp files/boot-svardos.img $USBIMG
mcopy -sQm -i "$USBIMG@@32256" "$FLOPROOT/"* ::/
for p in $ALLPKGS ; do
  mcopy -mi "$USBIMG@@32256" "$CDROOT/$p.svp" ::/
done

# compress the USB image
zip -mj9 "$PUBDIR/svardos-$CURDATE-usb.zip" "$USBIMG"

echo
echo "### Generating ISO CD image"
echo

CDISO="$PUBDIR/svardos-$CURDATE-cd.iso"
CDZIP="$PUBDIR/svardos-$CURDATE-cd.zip"

$GENISOIMAGE -input-charset cp437 -b boot.img -iso-level 1 -f -V SVARDOS -o "$CDISO" "$CDROOT/boot.img"

# compress the ISO
zip -mj9 "$CDZIP" "$CDISO"

# cleanup temporary things
if [ "x$2" != "xnoclean" ] ; then
  echo
  echo "### Clenup of temporary directories:"
  echo "# $CDROOT"
  echo "# $FLOPROOT"
  echo
  rm -rf "$CDROOT" "$FLOPROOT"
fi

cd "$origdir"

echo
echo "### ALL DONE! ###"

exit 0

#!/bin/bash
#
# SvarDOS build script
# http://svardos.osdn.io
# Copyright (C) 2016-2022 Mateusz Viste
#
# This script generates the SvarDOS repository index and builds ISO CD images.
# It should be executed each time that a package has been modified, added or
# removed.
#
# usage: ./build.sh [noclean]
#

### parameters block starts here ############################################

PKGDIR=`realpath ./packages`
REPOROOT=`realpath ./website/repo`
BUILDIDX=`realpath ./buildidx/buildidx.php`
PUBDIR=`realpath ./website/download`
CDROOT=`realpath ./cdroot`
FLOPROOT=`realpath ./floproot`
CUSTFILES=`realpath ./files`

GENISOIMAGE=''    # can be mkisofs, genisoimage or empty for autodetection

### parameters block ends here ##############################################

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
COREPKGS="amb attrib chkdsk choice command cpidos debug deltree devload diskcopy display dosfsck edit fc fdapm fdisk find format help himemx kernel keyb keyb_lay label localcfg mem mode more move pkg pkgnet shsucdx sort tree"

# list of packages to be part of EXTRA (only sometimes installed, typically drivers)
EXTRAPKGS="pcntpk udvd2"

# all packages
ALLPKGS="$COREPKGS $EXTRAPKGS"


# function that builds the packages repository
function dorepo {
  # clear out the web repo and copy all zip files to it
  rm "$REPOROOT"/*.zip
  cp "$PKGDIR"/*.zip "$REPOROOT/"
  # now strip the sources from repo versions
  find "$REPOROOT/" -iname '*.zip' -exec zip "{}" -d "SOURCE/*" ';'
  find "$REPOROOT/" -iname '*.zip' -exec zip "{}" -d "source/*" ';'
  find "$REPOROOT/" -iname '*.zip' -exec zip "{}" -d "Source/*" ';'

  # build repo index
  php "$BUILDIDX" "$REPOROOT"
}


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
    if ! mcopy -mi "$4/disk$curdisk.img" "$CDROOT/$p.zip" ::/ ; then
      curdisk=$((curdisk+1))
      mformat -C -t $1 -h $2 -s $3 -v SVARDOS -i "$4/disk$curdisk.img"
      mcopy -mi "$4/disk$curdisk.img" "$CDROOT/$p.zip" ::/
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
  zip -9 -rmj "$PUBDIR/svardos-floppy-$4k.zip" $4/*

  # clean up
  rmdir $4
}


### actual code flow starts here ############################################

# check presence of the buildidx tool
if [ ! -f "$BUILDIDX" ] ; then
  echo "buildidx not found at $BUILDIDX"
  exit 1
fi

# remember where I am, so I can get back here once all is done
origdir=`pwd`

mkdir "$CDROOT"
mkdir "$FLOPROOT"

# build the repo (also builds the listing.txt file)
dorepo

# add CORE packages to CDROOT + create the list of packages on floppy
for pkg in $COREPKGS ; do
  cp "$REPOROOT/$pkg.zip" "$CDROOT/"
  echo "$pkg" >> "$FLOPROOT/install.lst"
done

# add EXTRA packages to CDROOT (but not in the list of packages so instal won't install them by default)
for pkg in $EXTRAPKGS ; do
  cp "$REPOROOT/$pkg.zip" "$CDROOT/"
done


# prepare the content of the boot (install) floppy
cp "install/install.com" "$FLOPROOT/"
cp "install/nls/"install.?? "$FLOPROOT/"
cp -r "$CUSTFILES/floppy/"* "$FLOPROOT/"
unzip -Cj packages/cpidos.zip 'cpi/ega*.cpx' -d "$FLOPROOT/"
unzip -Cj packages/command.zip bin/command.com -d "$FLOPROOT/"
unzip -Cj packages/display.zip bin/display.exe -d "$FLOPROOT/"
unzip -Cj packages/edit.zip bin/edit.exe -d "$FLOPROOT/"
unzip -Cj packages/fdapm.zip bin/fdapm.com -d "$FLOPROOT/"
unzip -Cj packages/fdisk.zip bin/fdisk.exe bin/fdiskpt.ini -d "$FLOPROOT/"
unzip -Cj packages/format.zip bin/format.exe -d "$FLOPROOT/"
unzip -Cj packages/kernel.zip bin/kernel.sys bin/sys.com -d "$FLOPROOT/"
unzip -Cj packages/mem.zip bin/mem.exe -d "$FLOPROOT/"
unzip -Cj packages/mode.zip bin/mode.com -d "$FLOPROOT/"
unzip -Cj packages/more.zip bin/more.exe -d "$FLOPROOT/"
unzip -Cj packages/pkg.zip bin/pkg.exe -d "$FLOPROOT/"

# build the boot (CD) floppy image
export MTOOLS_NO_VFAT=1
#mformat -C -f 2880 -v SVARDOS -B "$CUSTFILES/floppy.mbr" -i "$CDROOT/boot.img"
#mcopy -sQm -i "$CDROOT/boot.img" "$FLOPROOT/"* ::/

# prepare images for floppies in different sizes (args are C H S SIZE)
prep_flop 80 2 36 2880 "$CDROOT/boot.img"
prep_flop 80 2 18 1440
prep_flop 80 2 15 1200
prep_flop 80 2  9  720
#prep_flop 96 64 32 98304 "$PUBDIR/svardos-zip100.img" # ZIP 100M (for USB boot in "USB-ZIP mode")

# prepare the DOSEMU boot zip
DOSEMUDIR='dosemu-prep-files'
mkdir "$DOSEMUDIR"
# INSTALL.BAT
echo 'IF NOT EXIST C:\TMP\NUL MKDIR C:\TMP' >> "$DOSEMUDIR/install.bat"
echo 'mkdir %DOSDIR%' >> "$DOSEMUDIR/install.bat"
echo 'mkdir %DOSDIR%\cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO # pkg config file - specifies locations where packages should be installed >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO DIR PROGS C:\ >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO DIR GAMES C:\ >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO DIR DRIVERS C:\DRIVERS\ >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
echo 'ECHO DIR DEVEL C:\DEVEL\ >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
for p in $COREPKGS ; do
  cp "$CDROOT/$p.zip" "$DOSEMUDIR/"
  echo "pkg install $p.zip" >> "$DOSEMUDIR/install.bat"
  echo "del $p.zip" >> "$DOSEMUDIR/install.bat"
done
echo 'ECHO my_ip = dhcp >> %DOSDIR%\CFG\WATTCP.CFG' >> "$DOSEMUDIR/install.bat"
echo 'del pkg.exe' >> "$DOSEMUDIR/install.bat"
echo 'ECHO SHELLHIGH=C:\SVARDOS\BIN\COMMAND.COM /P >> C:\CONFIG.SYS' >> "$DOSEMUDIR/install.bat"
echo 'ECHO.' >> "$DOSEMUDIR/install.bat"
echo 'ECHO -------------------------' >> "$DOSEMUDIR/install.bat"
echo 'ECHO  SVARDOS SETUP COMPLETED ' >> "$DOSEMUDIR/install.bat"
echo 'ECHO -------------------------' >> "$DOSEMUDIR/install.bat"
echo 'ECHO.' >> "$DOSEMUDIR/install.bat"
unzip -Cj packages/kernel.zip bin/kernel.sys -d "$DOSEMUDIR/"
unzip -Cj packages/command.zip bin/command.com -d "$DOSEMUDIR/"
unzip -Cj packages/pkg.zip bin/pkg.exe -d "$DOSEMUDIR/"
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
echo 'SET TEMP=C:\TMP' >> "$DOSEMUDIR/autoexec.bat"
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
zip -rm9jk "$PUBDIR/svardos-dosemu.zip" "$DOSEMUDIR"
rmdir "$DOSEMUDIR"

# prepare the USB bootable image
USBIMG=$PUBDIR/svardos-usb.img
cp files/boot-svardos.img $USBIMG
mcopy -sQm -i "$USBIMG@@32256" "$FLOPROOT/"* ::/
for p in $ALLPKGS ; do
  mcopy -mi "$USBIMG@@32256" "$CDROOT/$p.zip" ::/
done

# compress the USB image
rm -f "$PUBDIR/svardos-usb.zip"
zip -mj9 "$PUBDIR/svardos-usb.zip" "$USBIMG"

# prepare the USB-ZIP bootable image
#USBZIPIMG=$PUBDIR/svardos-usbzip.img
#cat files/usb-zip.mbr "$PUBDIR/svardos-zip100.img" > $USBZIPIMG

CDISO="$PUBDIR/svardos-cd.iso"
CDZIP="$PUBDIR/svardos-cd.zip"

# delete previous (if any) iso
echo "cleaning up old versions..."
rm -f "$CDISO" "$CDZIP"

$GENISOIMAGE -input-charset cp437 -b boot.img -iso-level 1 -f -V SVARDOS -o "$CDISO" "$CDROOT/boot.img"

# compress the ISO
zip -mj9 "$CDZIP" "$CDISO"

# cleanup temporary things
if [ "x$1" != "xnoclean" ] ; then
  rm -rf "$CDROOT" "$FLOPROOT"
fi

cd "$origdir"

#cd svnlschk
#./webgen.sh
#cd ..

echo "ALL DONE!"

exit 0

#!/bin/bash
#
# SvarDOS build script
# http://svardos.org
# Copyright (C) 2016-2022 Mateusz Viste
#
# This script builds floppy and CD images. It should be executed each time that
# a CORE package has been modified or the build script changed. This is usually
# done by the cron.sh script, itself called by a cron job.
#
# usage: ./build.sh outputdir buildver [noclean] > logfile
#

### parameters block starts here ############################################

REPOROOT=`realpath ./packages`
CUSTFILES=`realpath ./files`

GENISOIMAGE=''    # can be mkisofs, genisoimage or empty for autodetection

### parameters block ends here ##############################################

# look for mandatory output dir and build id
if [ "x$2" == "x" ] ; then
  echo "usage: build.sh outputdir buildver [noclean]"
  exit 1
fi
CURDATE="$2"
PUBDIR=`realpath "$1"`

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
# $5 working directory (for temporary files etc)
# $6 where to put a copy of the image (optional)
function prep_flop {
  WORKDIR="$5/$4"
  mkdir "$WORKDIR"
  mformat -C -t $1 -h $2 -s $3 -v SVARDOS -B "$CUSTFILES/floppy.mbr" -i "$WORKDIR/disk1.img"
  mcopy -sQm -i "$WORKDIR/disk1.img" "$FLOPROOT/"* ::/

  # now populate the floppies
  curdisk=1
  LIST=$ALLPKGS

  while [ ! -z "$LIST" ] ; do

    unset PENDING
    for p in $LIST ; do
      # if copy fails, then probably the floppy is full - try other packages
      # but remember all that fails so they will be retried on a new floppy
      if ! mcopy -mi "$WORKDIR/disk$curdisk.img" "$CDROOT/$p.svp" ::/ ; then
        PENDING="$PENDING $p"
      fi
    done

    LIST="$PENDING"
    # if there are any pending items, then create a new floppy and try pushing pending packages to it
    if [ ! -z "$PENDING" ] ; then
      curdisk=$((curdisk+1))
      mformat -C -t $1 -h $2 -s $3 -v SVARDOS -i "$WORKDIR/disk$curdisk.img"
    fi

  done

  # add a short readme
  echo "This directory contains a set of $curdisk floppy images of the SvarDOS distribution in the $4 KB floppy format." > "$WORKDIR/readme.txt"
  echo "" >> "$WORKDIR/readme.txt"
  echo "These images are raw floppy disk dumps. To write them on an actual floppy disk, you have to use a low-level sector copying tool, like dd." >> "$WORKDIR/readme.txt"
  echo "" >> "$WORKDIR/readme.txt"
  echo "Latest SvarDOS version is available on the project's homepage: http://svardos.org" >> "$WORKDIR/readme.txt"

  unix2dos "$WORKDIR/readme.txt"

  # make a copy of the image, if requested
  if [ "x$6" != "x" ] ; then
    cp "$WORKDIR/disk1.img" $6
  fi

  # zip the images (and remove them at the same time)
  rm -f "$PUBDIR/svardos-floppy-$4k.zip"
  zip -9 -rmj "$PUBDIR/svardos-$CURDATE-floppy-$4k.zip" "$WORKDIR"/*

  # clean up
  rmdir "$WORKDIR"
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

# prepare the content of the boot (install) floppy, unzipping everything
# in lowercase (-L) to avoid any case mismatching later in the build process
cp -r "$CUSTFILES/floppy/"* "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/cpidos.svp" 'cpi/ega*.cpx' -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/svarcom.svp" command.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/display.svp" bin/display.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/edit.svp" bin/edit.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/fdapm.svp" bin/fdapm.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/fdisk.svp" bin/fdisk.exe bin/fdiskpt.ini -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/format.svp" bin/format.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/kernel.svp" bin/kernel.sys bin/sys.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/mem.svp" bin/mem.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/mode.svp" bin/mode.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/more.svp" bin/more.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOT/core/pkg.svp" bin/pkg.exe -d "$FLOPROOT/"

# generate a simple autoexec.bat file
echo '@ECHO OFF' > "$FLOPROOT/autoexec.bat"
echo '' >> "$FLOPROOT/autoexec.bat"
echo 'REM Load DISPLAY driver if present' >> "$FLOPROOT/autoexec.bat"
echo 'IF EXIST DISPLAY.EXE DISPLAY CON=(EGA,,1)' >> "$FLOPROOT/autoexec.bat"
echo '' >> "$FLOPROOT/autoexec.bat"
echo 'FDAPM APMDOS' >> "$FLOPROOT/autoexec.bat"
echo '' >> "$FLOPROOT/autoexec.bat"
echo 'ECHO.' >> "$FLOPROOT/autoexec.bat"
echo 'ECHO  ********************' >> "$FLOPROOT/autoexec.bat"
echo 'ECHO   WELCOME TO SVARDOS' >> "$FLOPROOT/autoexec.bat"
echo 'ECHO  ********************' >> "$FLOPROOT/autoexec.bat"
echo "ECHO  build: $CURDATE" >> "$FLOPROOT/autoexec.bat"
echo 'ECHO.' >> "$FLOPROOT/autoexec.bat"
echo '' >> "$FLOPROOT/autoexec.bat"
echo "INSTALL $CURDATE" >> "$FLOPROOT/autoexec.bat"
unix2dos "$FLOPROOT/autoexec.bat"


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
echo "### Creating floppy images"
echo

# build the boot (CD) floppy image
export MTOOLS_NO_VFAT=1

# prepare images for floppies in different sizes (args are C H S SIZE)
prep_flop 80 2 36 2880 "$PUBDIR" "$CDROOT/boot.img"
prep_flop 80 2 18 1440 "$PUBDIR"
prep_flop 80 2 15 1200 "$PUBDIR"
prep_flop 80 2  9  720 "$PUBDIR"

# special case for 360K diskettes: some files must be deleted to make some room,
# for this reason the 360K floppy must be generated as last (after all other
# floppies and after the USB image)
rm "$FLOPROOT"/*.cpx
rm "$FLOPROOT"/install.lng
rm "$FLOPROOT"/display.exe
rm "$FLOPROOT"/mode.com
rm "$FLOPROOT"/edit.*
#
prep_flop 40 2  9  360 "$PUBDIR"


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
echo 'DEL C:\CONFIG.SYS' >> "$DOSEMUDIR/install.bat"
echo 'COPY C:\CONFIG.NEW C:\CONFIG.SYS' >> "$DOSEMUDIR/install.bat"
echo 'DEL C:\CONFIG.NEW' >> "$DOSEMUDIR/install.bat"
echo 'SET COMSPEC=C:\COMMAND.COM' >> "$DOSEMUDIR/install.bat"
echo 'DEL C:\CMD.COM' >> "$DOSEMUDIR/install.bat"
echo 'ECHO.' >> "$DOSEMUDIR/install.bat"
echo 'ECHO -------------------------' >> "$DOSEMUDIR/install.bat"
echo 'ECHO  SVARDOS SETUP COMPLETED' >> "$DOSEMUDIR/install.bat"
echo 'ECHO   PLEASE RESTART DOSEMU' >> "$DOSEMUDIR/install.bat"
echo 'ECHO -------------------------' >> "$DOSEMUDIR/install.bat"
echo 'ECHO.' >> "$DOSEMUDIR/install.bat"
unzip -Cj "$REPOROOT/core/kernel.svp" bin/kernel.sys -d "$DOSEMUDIR/"
unzip -CLj "$REPOROOT/core/svarcom.svp" command.com -d "$DOSEMUDIR/"
mv "$DOSEMUDIR/command.com" "$DOSEMUDIR/cmd.com"
unzip -Cj "$REPOROOT/core/pkg.svp" bin/pkg.exe -d "$DOSEMUDIR/"
# CONFIG.SYS
echo 'FILES=25' >> "$DOSEMUDIR/config.sys"
echo 'DOS=HIGH,UMB' >> "$DOSEMUDIR/config.sys"
echo 'DOSDATA=UMB' >> "$DOSEMUDIR/config.sys"
echo 'DEVICE=D:\dosemu\emufs.sys' >> "$DOSEMUDIR/config.sys"
echo 'DEVICE=D:\dosemu\umb.sys' >> "$DOSEMUDIR/config.sys"
echo 'DEVICEHIGH=D:\dosemu\ems.sys' >> "$DOSEMUDIR/config.sys"
echo 'INSTALL=D:\dosemu\emufs.com' >> "$DOSEMUDIR/config.sys"
cp "$DOSEMUDIR/config.sys" "$DOSEMUDIR/config.new"
echo 'SHELL=C:\CMD.COM /P' >> "$DOSEMUDIR/config.sys"
echo 'SHELL=C:\COMMAND.COM /P' >> "$DOSEMUDIR/config.new"
# AUTOEXEC.BAT
echo "@ECHO OFF" >> "$DOSEMUDIR/autoexec.bat"
echo 'SET DOSDIR=C:\SVARDOS' >> "$DOSEMUDIR/autoexec.bat"
echo 'SET WATTCP.CFG=%DOSDIR%\CFG' >> "$DOSEMUDIR/autoexec.bat"
echo 'SET DIRCMD=/p/ogne' >> "$DOSEMUDIR/autoexec.bat"
echo 'SET TEMP=C:\TEMP' >> "$DOSEMUDIR/autoexec.bat"
echo 'PATH %DOSDIR%\BIN' >> "$DOSEMUDIR/autoexec.bat"
echo "" >> "$DOSEMUDIR/autoexec.bat"
echo 'IF NOT EXIST INSTALL.BAT GOTO NORMBOOT' >> "$DOSEMUDIR/autoexec.bat"
echo "REM *** this is a one-time setup script used only during first initialization ***" >> "$DOSEMUDIR/autoexec.bat"
echo 'CALL INSTALL.BAT' >> "$DOSEMUDIR/autoexec.bat"
echo 'DEL INSTALL.BAT' >> "$DOSEMUDIR/autoexec.bat"
echo 'GOTO ENDOFFILE' >> "$DOSEMUDIR/autoexec.bat"
echo "" >> "$DOSEMUDIR/autoexec.bat"
echo ":NORMBOOT" >> "$DOSEMUDIR/autoexec.bat"
echo "ECHO." >> "$DOSEMUDIR/autoexec.bat"
echo "ECHO Welcome to SvarDOS (powered by DOSEMU)! Type HELP if you are lost." >> "$DOSEMUDIR/autoexec.bat"
echo "ECHO." >> "$DOSEMUDIR/autoexec.bat"
echo ":ENDOFFILE" >> "$DOSEMUDIR/autoexec.bat"
rm -f "$PUBDIR/svardos-dosemu.zip"
zip -rm9jk "$PUBDIR/svardos-$CURDATE-dosemu.zip" "$DOSEMUDIR"
rmdir "$DOSEMUDIR"

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
  echo "### Cleanup of temporary directories:"
  echo "# $CDROOT"
  echo "# $FLOPROOT"
  echo
  rm -rf "$CDROOT" "$FLOPROOT"
fi

cd "$origdir"

echo
echo "### ALL DONE! ###"

exit 0

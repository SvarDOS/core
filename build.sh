#!/bin/bash
#
# SvarDOS build script
# http://svardos.org
#
# Copyright (C) 2016-2024 Mateusz Viste
#
# This script builds floppy and CD images. It should be executed each time that
# a CORE package has been modified or the build script changed. This is usually
# done by the cron.sh script, itself called by a cron job.
#
# usage: ./build.sh outputdir buildver [noclean] > logfile
#

### parameters block starts here ############################################

REPOROOT=`realpath ./packages`
REPOROOTCORE=`realpath ./packages-core`
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


# list of packages to be part of CORE (always installed), sort them from
# biggest to smallest to try utilizing the floppy disk space as efficiently
# as possible. -L means 'dereference', ie. look at the filesize of the target
# file, not the symlink.
COREPKGS=`ls -1LS 'packages-core' | grep -o '^[a-zA-Z0-9]*'`



# prepares image for floppy sets of:
# $1 cylinders
# $2 heads (sides)
# $3 sectors per track
# $4 size
# $5 working directory (for temporary files etc)
# $6 name of the set (eg. "1440k" or "1440k-EN")
# $7 list of packages
# $8 where to put a copy of the image (optional)
function prep_flop {
  WORKDIR="$5/$6"
  LIST=$7
  DISKPREFIX="svdos-$6-disk"
  mkdir "$WORKDIR"
  mformat -C -t $1 -h $2 -s $3 -v $CURDATE -B "$CUSTFILES/floppy.mbr" -i "$WORKDIR/$DISKPREFIX-1.img"

  # copy basic files (installer, startup files...)
  mcopy -sQm -i "$WORKDIR/$DISKPREFIX-1.img" "$FLOPROOT/"* ::/

  # generate the INSTALL.LST file
  for pkg in $7 ; do
    echo "$pkg" >> "$WORKDIR/install.lst"
  done
  mcopy -i "$WORKDIR/$DISKPREFIX-1.img" "$WORKDIR/install.lst" ::/
  rm "$WORKDIR/install.lst"

  # now populate the floppies with *.svp packages
  curdisk=1

  while [ ! -z "$LIST" ] ; do

    unset PENDING
    for p in $LIST ; do
      # if copy fails, then probably the floppy is full - try other packages
      # but remember all that fails so they will be retried on a new floppy
      if ! mcopy -mi "$WORKDIR/$DISKPREFIX-$curdisk.img" "$CDROOT/$p.svp" ::/ ; then
        PENDING="$PENDING $p"
      fi
    done

    LIST="$PENDING"
    # if there are any pending items, then create a new floppy and try pushing pending packages to it
    if [ ! -z "$PENDING" ] ; then
      curdisk=$((curdisk+1))
      mformat -C -t $1 -h $2 -s $3 -v SVARDOS -i "$WORKDIR/$DISKPREFIX-$curdisk.img"
    fi

  done

  # add a short readme
  echo "This directory contains a set of $curdisk floppy images of the SvarDOS distribution in the $4 KB floppy format." > "$WORKDIR/readme.txt"
  echo "" >> "$WORKDIR/readme.txt"
  echo "These images are raw floppy disk dumps. To write them on an actual floppy disk, you have to use a low-level sector copying tool, like dd." >> "$WORKDIR/readme.txt"
  echo "" >> "$WORKDIR/readme.txt"
  echo "Latest SvarDOS version is available on the project's homepage: http://svardos.org" >> "$WORKDIR/readme.txt"

  unix2dos "$WORKDIR/readme.txt"

  # make a copy of the (first) image, if requested
  if [ "x$8" != "x" ] ; then
    cp "$WORKDIR/$DISKPREFIX-1.img" $8
  fi

  # zip the images (and remove them at the same time)
  zip -9 -rmj "$PUBDIR/svardos-$CURDATE-floppy-$6.zip" "$WORKDIR"/*

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
  cp "$REPOROOTCORE/$pkg.svp" "$CDROOT/"
done

# add some extra packages to CDROOT but not in the list of packages to install
cp "$REPOROOT/pcntpk.svp" "$CDROOT/"
cp "$REPOROOT/videcdd-2.14.svp" "$CDROOT/videcdd.svp"
cp "$REPOROOT/provox-7.05.svp" "$CDROOT/provox.svp"

#


echo
echo "### Populating the floppy root at $FLOPROOT"
echo

# prepare the content of the boot (install) floppy, unzipping everything
# in lowercase (-L) to avoid any case mismatching later in the build process
cp -r "$CUSTFILES/floppy/"* "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/cpidos.svp" 'cpi/ega.cpx' -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/cpidos.svp" 'cpi/ega3.cpx' -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/cpidos.svp" 'cpi/ega6.cpx' -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/cpidos.svp" 'cpi/ega9.cpx' -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/cpidos.svp" 'cpi/ega10.cpx' -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/svarcom.svp" command.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/display.svp" bin/display.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/fdapm.svp" bin/fdapm.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/fdisk.svp" bin/fdisk.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/format.svp" bin/format.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/kernledr.svp" kernel.sys -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/mem.svp" bin/mem.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/mode.svp" bin/mode.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/more.svp" bin/more.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/pkg.svp" bin/pkg.exe -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/sved.svp" bin/sved.com -d "$FLOPROOT/"
unzip -CLj "$REPOROOTCORE/sys.svp" bin/sys.com -d "$FLOPROOT/"

# generate a simple AUTOEXEC.BAT
cat <<EOF > "$FLOPROOT/autoexec.bat"
@ECHO OFF
DISPLAY CON=(EGA,,1)
FDAPM ADV:REG

ECHO.
ECHO  ********************
ECHO   WELCOME TO SVARDOS
ECHO  ********************
ECHO  build: $CURDATE
ECHO.

REM Load PROVOX screen reader if present
IF NOT EXIST PROVOX7.EXE GOTO INSTALL
CLS
ECHO This svardos build comes with the provox screen reader preinstalled
ECHO.
ECHO It outputs speech to a Braille n Speak device that has to be connected to
ECHO the first serial port of your computer
ECHO.
ECHO The installation of the SvarDOS system is about to begin
ECHO Press any key to continue
ECHO.
PROVOX7.EXE
PV7.EXE INIT BNS > NUL
PAUSE

:INSTALL
INSTALL
EOF
unix2dos "$FLOPROOT/autoexec.bat"

# generate a simple CONFIG.SYS
cat <<EOF > "$FLOPROOT/config.sys"
LASTDRIVE=Z
FILES=8
BUFFERS=10
HISTORY=ON,128
SHELL=COMMAND.COM /e:512 /p
EOF
unix2dos "$FLOPROOT/config.sys"


echo
echo "### Computing the USB image"
echo

# prepare the USB bootable image
USBIMG=$PUBDIR/svardos-usb.img
cp files/boot-svardos.img $USBIMG
mlabel -i "$USBIMG@@32256" ::$CURDATE
mcopy -sQm -i "$USBIMG@@32256" "$FLOPROOT/"* ::/
for p in $COREPKGS ; do
  mcopy -mi "$USBIMG@@32256" "$CDROOT/$p.svp" ::/
done

# compress the USB image
zip -mj9 "$PUBDIR/svardos-$CURDATE-usb.zip" "$USBIMG"


echo
echo "### Creating floppy images"
echo
echo "You might notice a lot of DISK FULL warnings below. Do not worry, these"
echo "are expected and are perfectly normal. It is a side effect of trying to"
echo "fit as many packages as possible on the floppy sets."
echo

# build the boot (CD) floppy image
export MTOOLS_NO_VFAT=1

# prepare images for floppies in different sizes (args are C H S SIZE)
prep_flop 80 2 36 2880 "$PUBDIR" "2.88M" "$COREPKGS pcntpk videcdd" "$CDROOT/boot.img"
prep_flop 80 2 18 1440 "$PUBDIR" "1.44M" "$COREPKGS pcntpk"
prep_flop 80 2 15 1200 "$PUBDIR" "1.2M" "$COREPKGS"
prep_flop 80 2  9  720 "$PUBDIR" "720K" "$COREPKGS"
prep_flop 40 2  9  360 "$PUBDIR" "360K" "$COREPKGS"

# BNS-enabled (screen reader) 2.88M build
unzip -CLj "$CDROOT/provox.svp" drivers/provox/provox7.exe -d "$FLOPROOT/"
unzip -CLj "$CDROOT/provox.svp" drivers/provox/pv7.exe -d "$FLOPROOT/"
prep_flop 80 2 36 2880 "$PUBDIR" "2.88M-BNS" "$COREPKGS pcntpk videcdd provox" "$CDROOT/bootbns.img"
rm "$FLOPROOT/provox7.exe"
rm "$FLOPROOT/pv7.exe"



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
echo 'ECHO DIR BIN %DOSDIR% >> %DOSDIR%\cfg\pkg.cfg' >> "$DOSEMUDIR/install.bat"
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
unzip -Cj "$REPOROOTCORE/kernledr.svp" kernel.sys -d "$DOSEMUDIR/"
unzip -CLj "$REPOROOTCORE/svarcom.svp" command.com -d "$DOSEMUDIR/"
mv "$DOSEMUDIR/command.com" "$DOSEMUDIR/cmd.com"
unzip -Cj "$REPOROOTCORE/pkg.svp" bin/pkg.exe -d "$DOSEMUDIR/"
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
echo 'PATH %DOSDIR%' >> "$DOSEMUDIR/autoexec.bat"
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
echo "### Generating ISO CD images"
echo

# normal
CDISO="$PUBDIR/svardos-$CURDATE-cd.iso"
CDZIP="$PUBDIR/svardos-$CURDATE-cd.zip"
$GENISOIMAGE -input-charset cp437 -b boot.img -iso-level 1 -f -V SVARDOS -o "$CDISO" "$CDROOT/boot.img"
zip -mj9 "$CDZIP" "$CDISO"

# BNS-enabled
CDISO="$PUBDIR/svardos-$CURDATE-cd-bns.iso"
CDZIP="$PUBDIR/svardos-$CURDATE-cd-bns.zip"
$GENISOIMAGE -input-charset cp437 -b bootbns.img -iso-level 1 -f -V SVARDOS -o "$CDISO" "$CDROOT/bootbns.img"
zip -mj9 "$CDZIP" "$CDISO"



###############################################################################
# compute the svardos stub, useful for migrating alien DOS systems to SvarDOS #
###############################################################################

echo
echo "### generating the pkgstub zip archive"

mkdir "$PUBDIR"/pkgstub
unzip -jC "$REPOROOTCORE"/pkgnet.svp bin/pkgnet.exe -d "$PUBDIR"/pkgstub
unzip -jC "$REPOROOTCORE"/pkg.svp bin/pkg.exe -d "$PUBDIR"/pkgstub

cat <<EOF > "$PUBDIR"/pkgstub/readme.txt

        SvarDOS stub, or how to plant a SvarDOS seed in a foreign land
        --------------------------------------------------------------

This archive contains files that allow to install a SvarDOS stub within
another DOS system. This makes it possible to use the SvarDOS online repository
of packages on non-SvarDOS systems, assuming you have a network card connected
to the internet and a suitable packet driver.

If you do not have internet connectivity, then you will still be able to
install SvarDOS packages (*.SVP) once you copy them to your PC. You can fetch
SvarDOS packages at <http://svardos.org>.

Follow the guide now:

=======================================
MKDIR C:\\SVARDOS
MKDIR C:\\SVARDOS\\CFG

SET WATTCP.CFG=C:\\SVARDOS\\CFG
SET DOSDIR=C:\\SVARDOS
SET LANG=EN
SET PATH=%PATH%;C:\SVARDOS

COPY *.EXE C:\\SVARDOS\\
COPY *.CFG C:\\SVARDOS\CFG\\
=======================================

If in doubt, reach out to us at <http://svardos.org>.
EOF

cat <<EOF > "$PUBDIR"/pkgstub/wattcp.cfg
# Rely on DHCP by default
my_ip = dhcp

# modify (and uncomment) these if you have a fix IP setup:
#
#my_ip = 0.0.0.0  ; IP address
#netmask = 255.255.255.0  ; netmask
#nameserver = 0.0.0.0  ; primary DNS, mandatory if not using DHCP
#nameserver = 0.0.0.0  ; secondary DNS, optional
#gateway = 0.0.0.0  ; default gateway
EOF

cat <<EOF > "$PUBDIR"/pkgstub/pkg.cfg
# pkg config file - specifies locations where SvarDOS packages will be installed

# SvarDOS core files
DIR BIN C:\\SVARDOS\\

# General location for programs
DIR PROGS C:\\

# Games
DIR GAMES C:\\

# Drivers
DIR DRIVERS C:\\DRIVERS\\

# Development tools
DIR DEVEL C:\\DEVEL\\
EOF

zip -m -k9jr "$PUBDIR"/svarstub.zip "$PUBDIR"/pkgstub
rmdir "$PUBDIR"/pkgstub



###############################################################################
# cleanup all temporary things                                                #
###############################################################################

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

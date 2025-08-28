#!/bin/bash
#
# this script looks for the presence of /tmp/svardos_repo_changed.flag file
# and rebuilds the packages index if it exists (then deletes the flag).
#
# it is supposed to be called periodically from within a cron job.
#
# the /tmp/svardos_repo_changed.flag file is expected to be created by an
# svn post-commit hook whenever something in the svn repo changes.
#
# bash is required because I use the internal bash "read" command.

REPOFLAGFILE="/tmp/svardos_repo_changed.flag"
REBUILDFLAGFILE="/tmp/svardos_rebuild_please.flag"
SVNREPODIR="/srv/svardos"

# exit if repo did not change
if [ ! -f "$REPOFLAGFILE" ] ; then
  exit 0
fi

# delete the flag files as soon as possible to limit the time of possible
# collisions
rm "$REPOFLAGFILE"

NEEDTOREBUILD=0
if [ -f "$REBUILDFLAGFILE" ] ; then
  rm "$REBUILDFLAGFILE"
  NEEDTOREBUILD=1
fi


# refresh the local copy of the repo and rebuild packages index
svn up "$SVNREPODIR"
rm -rf "$SVNREPODIR/packages/latest"
php "$SVNREPODIR/buildidx/buildidx.php" "$SVNREPODIR/packages/" > "$SVNREPODIR/packages/_buildidx.log"


###############################################################################
#                                                                             #
# build the ISO that contains all latest packages from the repo               #
# (and make it bootable using latest STABLE boot floppy)                      #
#                                                                             #
###############################################################################

read -r STABLE < "$SVNREPODIR/website/default_build.txt"
unzip -o "$SVNREPODIR/website/download/$STABLE/svardos-$STABLE-floppy-2.88M.zip" svdos-2.88M-disk-1.img -d /tmp/
mv /tmp/svdos-2.88M-disk-1.img "$SVNREPODIR/packages/latest/boot.img"
mkisofs -input-charset cp437 -b boot.img -hide boot.img -hide boot.catalog -iso-level 1 -f -V SVARDOS_REPO -o "$SVNREPODIR/website/repo/sv-repo.tmp" "$SVNREPODIR"/packages/latest/*
rm -f "$SVNREPODIR/packages/latest/boot.img"
mv "$SVNREPODIR/website/repo/sv-repo.tmp" "$SVNREPODIR/website/repo/sv-repo.iso"
md5sum "$SVNREPODIR/website/repo/sv-repo.iso" > "$SVNREPODIR/website/repo/sv-repo.iso.md5"


# do I need to rebuild the install images as well?
if [ "$NEEDTOREBUILD" -ne 0 ] ; then
  CURDATE=`date +%Y%m%d`
  DESTDIR="website/download/$CURDATE"
  cd "$SVNREPODIR"
  rm -rf "$DESTDIR"
  mkdir "$DESTDIR"
  ./build.sh "$DESTDIR" "$CURDATE" > "$DESTDIR/build.log" 2>&1
fi

exit 0

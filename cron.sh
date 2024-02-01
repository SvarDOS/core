#!/bin/sh
#
# this script looks for the presence of /tmp/svardos_repo_changed.flag file
# and rebuilds the packages index if it exists (then deletes the flag).
#
# it is supposed to be called periodically from within a cron job.
#
# the /tmp/svardos_repo_changed.flag file is expected to be created by an
# svn post-commit hook whenever something in the svn repo changes.


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

#!/bin/sh
#
# this script looks for the presence of /tmp/svardos_repo_changed.flag file
# and rebuilds the packages index if it exists (then deletes the flag).
#
# it is supposed to be called periodically from within a cron job.
#
# the /tmp/svardos_repo_changed.flag file is expected to be created by an
# svn post-commit hook whenever something in the svn repo changes.


FLAGFILE="/tmp/svardos_repo_changed.flag"
SVNREPODIR="/srv/svardos"

# if flag does not exist, exit
if [ ! -f "$FLAGFILE" ] ; then
  exit 0
fi


# repo changed! delete the flag file, refresh the local copy of the repo and rebuild packages index
rm "$FLAGFILE"
svn up "$SVNREPODIR"
php "$SVNREPODIR/buildidx/buildidx.php" "$SVNREPODIR/packages/" > "$SVNREPODIR/packages/_buildidx.log"

exit 0

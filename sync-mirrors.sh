#!/bin/sh

# SvarDOS subversion repositories are mirrored for extra safety. This script
# synchronizes the mirrors.
# This requires specific credentials, hence at this time it can be executed
# only by me (Mateusz).

# sync the system repo to the OSDN mirror
svnsync sync svn+ssh://mateuszviste@svn.osdn.net/svnroot/svardos-mirror

# sync the packages repo to my HelixTeam hub account
svnsync sync svn+ssh://hth@helixteamhub.cloud/mateuszviste/projects/svardos-mirror/repositories/subversion/packages

echo "done."

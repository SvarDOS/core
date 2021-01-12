#!/bin/sh
#
# SvarDOS build script
# http://svardos.osdn.io
# Copyright (C) 2016-2021 Mateusz Viste
#
# Synchronization of ISO files, website and repositories towards sourceforge.
#

set -e

# sync packages (with sources) to the osdn storage server
rsync -a --progress --delete packages mateuszviste@storage.osdn.net:/storage/groups/s/sv/svardos/

# sync the website (with repositories and downloads)
rsync -rtDOvz --delete --progress website/ mateuszviste@shell.osdn.net:/home/groups/s/sv/svardos/htdocs/

#!/bin/sh
#
# SvarDOS build script
# http://svardos.osdn.io
# Copyright (C) 2016-2021 Mateusz Viste
#
# Synchronization of ISO files, website and repositories towards sourceforge.
#

# sync the website (with repositories and downloads)
rsync -rtOvz --delete --progress website/ mateuszviste@shell.osdn.net:/home/groups/s/sv/svardos/htdocs/

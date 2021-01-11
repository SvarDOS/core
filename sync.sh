#!/bin/sh
#
# SvarDOS build script
# http://svardos.osdn.io
# Copyright (C) 2016-2021 Mateusz Viste
#
# Synchronization of ISO files, website and repositories towards sourceforge.
#

# sync ISO files to sourceforge download servers
#rsync -a --progress iso/* mateuszviste@storage.osdn.net:/storage/groups/s/sv/svardos/
#if [ $? -ne 0 ] ; then exit 1 ; fi

# sync the website (with repositories)
rsync -a --delete --progress website/ mateuszviste@shell.osdn.net/home/groups/s/sv/svardos/htdocs/


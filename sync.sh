#!/bin/sh
#
# Svarog386 build script
# http://svarog386.sourceforge.net
# Copyright (C) 2016-2018 Mateusz Viste
#
# Synchronization of ISO files, website and repositories towards sourceforge.
#

# sync ISO files to sourceforge download servers
#rsync -a --progress iso/* mv_fox@frs.sourceforge.net:/home/frs/project/svarog386/
#if [ $? -ne 0 ] ; then exit 1 ; fi

# sync the website (with repositories)
rsync -a --delete --progress website/ mv_fox@web.sourceforge.net:/home/project-web/svarog386/htdocs

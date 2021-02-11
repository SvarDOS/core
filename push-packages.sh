#!/bin/sh

# sync packages (with sources) to the osdn storage server
rsync -rtOvz --delete --progress packages mateuszviste@storage.osdn.net:/storage/groups/s/sv/svardos/

# -r    recurse into directories
# -t    preserve modification times
# -O    this tells rsync to omit directories when it is preserving modification times
# -v    more verbosity
# -z    compress file data during the transfer

#----------------------------------------------------------------------------

BUILDDIR=build/$CATEGORY/$NAME
INFODIR=build/appinfo
DISTFILE=tmp/$NAME.zip
SRCFILE=pkg/$NAME-$VERSION.zip
PKGDIR=pkg

#----------------------------------------------------------------------------

FETCH_CMD="curl -L"

ACTION="$1"

function clean() {
	echo "build.sh: cleaning"
	rm -rf build pkg tmp
}

function prepare_dirs() {
	echo "build.sh: prepare dirs"
	mkdir -p $BUILDDIR
	mkdir -p $INFODIR
	mkdir -p $PKGDIR
	mkdir tmp	
}

function fetch_distfiles() {
	echo "build.sh: fetching distribution files"
	$FETCH_CMD "$DISTURL" -o $DISTFILE || exit 1
}

function unpack_distfiles() {
	echo "build.sh: unpacking distribution files"
	unzip $DISTFILE -d $BUILDDIR || exit 1
}

function generate_appinfo() {
	echo "build.sh: generating $NAME.lsm"
	echo "version: $VERSION" | unix2dos >$INFODIR/$NAME.lsm
	echo "description: $DESCRIPTION" | unix2dos >>$INFODIR/$NAME.lsm
	if [ "$HWREQ" != "" ]; then
		echo "hwreq: $HWREQ" | unix2dos >>$INFODIR/$NAME.lsm
	fi
	if [ "$WARN" != "" ]; then
		echo "hwreq: $HWREQ" | unix2dos >>$INFODIR/$NAME.lsm
	fi	
}

function generate_package() {
	echo "build.sh: generating $NAME.svp"
	(cd build; 7zz a -r -mm=deflate -mx=9 -tzip ../$PKGDIR/$NAME-$VERSION.svp appinfo $CATEGORY) || exit 1
	ln -s $NAME-$VERSION.svp $PKGDIR/$NAME.svp

}

function fix_distfiles() {
	# adjust all file names to be all-lowercase

	DISTFILES=$(find "$BUILDDIR")
	for F in $DISTFILES; do
		BN=$(basename "$F")

		UCN=$(echo $BN | tr '[:upper:]' '[:lower:]')
		if [ "$BN" != "$UCN" ]; then
			echo "lower-casing $UCN"
			mv "$F" "${F}_"
			mv "${F}_" $(dirname "$F")/$UCN
		fi
	done
}

function fetch_sourcefiles() {
	echo "build.sh: fetching sources"
	if [ "$SRCURL" != "" ]; then
		$FETCH_CMD "$SRCURL" -o "$SRCFILE" || exit 1
		ln -s $NAME-$VERSION.zip $PKGDIR/$NAME.zip
	fi
}

function build_package() {
	echo "build.sh: building $NAME $VERSION SvarDOS package..."
	clean
	prepare_dirs
	fetch_distfiles
	unpack_distfiles
	generate_appinfo
	adjust_builddir
	fix_distfiles
	generate_package	
}

function build_source_package() {
	# this currently only fetches the source archive without
	# altering it at all
	fetch_sourcefiles
}

function usage() {
	echo "build.sh - build script for $NAME $VERSION"
	echo "usage: build.sh [clean]"
}

function run() {
	if [ -z "$ACTION" ]; then
		build_package
		build_source_package
	elif [ "$ACTION" = "clean" ]; then
		clean
	else
		usage
	fi
}

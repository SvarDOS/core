FETCH_CMD="curl -L"
ZIP_CMD="7zz a -r -mm=deflate -mx=9 -tzip"

EXTERNAL_CMDS="curl unix2dos 7zz"

#----------------------------------------------------------------------------

BUILDDIR=build/$CATEGORY/$NAME
INFODIR=build/appinfo
DISTFILE=tmp/$NAME.zip
SRCFILE=pkg/$NAME-$VERSION.zip
PKGDIR=pkg
REPODIR=../../../packages

#----------------------------------------------------------------------------

ACTION="$1"

function check_externals() {
	for CMD in $EXTERNAL_CMDS; do
	# check for tools
		if ! command -v $CMD > /dev/null 2>&1; then
			echo "error: executable $CMD not found"
			exit 1
		fi
	done	
}

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
	(echo "version: $VERSION" | unix2dos) >$INFODIR/$NAME.lsm
	(echo "description: $DESCRIPTION" | unix2dos) >>$INFODIR/$NAME.lsm
	if [ "$HWREQ" != "" ]; then
		(echo "hwreq: $HWREQ" | unix2dos) >>$INFODIR/$NAME.lsm
	fi
	if [ "$WARN" != "" ]; then
		(echo "hwreq: $HWREQ" | unix2dos) >>$INFODIR/$NAME.lsm
	fi	
}

function generate_package() {
	echo "build.sh: generating $NAME.svp"
	(cd build; $ZIP_CMD ../$PKGDIR/$NAME-$VERSION.svp appinfo $CATEGORY) || exit 1
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
	if [ "$SRCURL" != "" ]; then
		echo "build.sh: fetching sources"
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


function upgrade() {
	OLDVERSION=$(ls "$REPODIR/$NAME-"*.svp | sed "s|$REPODIR\/$NAME-||; s|.svp\$||" | sort | tail -n 1)
	if [ -z "$OLDVERSION" ]; then
		echo "build.sh: cannot determine old version"
		exit 1
	fi
	if [ ! -f "$PKGDIR/$NAME-$VERSION.svp" ]; then
		echo "build.sh: you first have to build $PKGDIR/$NAME-$VERSION.svp"
		exit 1
	fi
	echo "build.sh: upgrading $NAME $OLDVERSION -> $VERSION"
	if [ "$OLDVERSION" != "$VERSION" ]; then
		echo "build.sh: SVN copy version $OLDVERSION to $VERSION"
		(cd $REPODIR; svn cp "$NAME-$OLDVERSION.svp" "$NAME-$VERSION.svp") || exit 1
	fi
	echo "build.sh: copying version $VERSION to SVN repo"
	cp -a "$PKGDIR/$NAME-$VERSION.svp" "$REPODIR/$NAME-$VERSION.svp"

	# handle source package
	OLDSRCVER=$(ls "$REPODIR/$NAME-"*.zip | sed "s|$REPODIR\/$NAME-||; s|.zip\$||" | sort | tail -n 1)
	if [ -f "$PKGDIR/$NAME-$VERSION.zip" ]; then
		echo "build.sh: upgrading source package"

		if [ "$OLDSRCVER" != "$VERSION" ]; then
			echo "build.sh: SVN copy source of version $OLDSRCVER to $VERSION"
			(cd $REPODIR; svn cp "$NAME-$OLDSRCVER.zip" "$NAME-$VERSION.zip") || exit 1
		fi

		echo "build.sh: copying source of version $VERSION to SVN repo"
		cp -a "$PKGDIR/$NAME-$VERSION.zip" "$REPODIR/$NAME-$VERSION.zip"

		if [ -z "$OLDSRCVER" ]; then
			echo "build.sh: no previous source version, you must manually SVN add"
		fi
	fi
	echo "build.sh: done!"
}

function usage() {
	echo "build.sh - SvarDOS package build script for $NAME $VERSION"
	echo ""
	echo "usage: build.sh [build|clean|upgrade]"
	echo ""
	echo "\tbuild    build .SVP package"
	echo "\tclean    deletes build artifacts"
	echo "\tupgrade  copies generated package to the repo dir and updates SVN"
}

function run() {
	if [ "$ACTION" = "build" ]; then
		check_externals
		build_package
		build_source_package
	elif [ "$ACTION" = "clean" ]; then
		clean
	elif [ "$ACTION" = "upgrade" ]; then
		upgrade
	else
		usage
	fi
}

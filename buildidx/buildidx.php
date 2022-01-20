<?php /*

  SvarDOS repo index builder
  Copyright (C) Mateusz Viste 2012-2022

  buildidx computes an index tsv file for the SvarDOS repository.
  it must be executed pointing to a directory that stores packages (zip)
  files. buildidx will generate the index file and save it into the package
  repository.

  requires php-zip

  20 jan 2022: rewritten the code from ANSI C to PHP for easier maintenance
  13 feb 2021: 'title' LSM field is no longer looked after
  11 feb 2021: lsm headers are no longer checked, so it is compatible with the simpler lsm format used by SvarDOS
  13 jan 2021: removed the identification line, changed CRC32 to bsum, not creating the listing.txt file and stopped compressing index
  23 apr 2017: uncompressed index is no longer created, added CRC32 of zib (bin only) files, if present
  28 aug 2016: listing.txt is always written inside the repo dir (instead of inside current dir)
  27 aug 2016: accepting full paths to repos (starting with /...)
  07 dec 2013: rewritten buildidx in ANSI C89
  19 aug 2013: add a compressed version of the index file to repos (index.gz)
  22 jul 2013: creating a listing.txt file with list of packages
  18 jul 2013: writing the number of packaged into the first line of the lst file
  11 jul 2013: added a switch to 7za to make it case insensitive when extracting lsm files
  10 jul 2013: changed unzip calls to 7za (to handle cases when appinfo is compressed with lzma)
  04 feb 2013: added CRC32 support
  22 sep 2012: forked 1st version from FDUPDATE builder
*/

$PVER = "20220120";


// computes the BSD sum of a file and returns it
function file2bsum($fname) {
  $result = 0;

  $fd = fopen($fname, 'rb');
  if ($fd === false) return(0);

  while (!feof($fd)) {

    $buff = fread($fd, 1024 * 1024);

    $slen = strlen($buff);
    for ($i = 0; $i < $slen; $i++) {
      // rotr
      $result = ($result >> 1) | ($result << 15);
      // add and truncate to 16 bits
      $result += ord($buff[$i]);
      $result &= 0xffff;
    }
  }

  fclose($fd);
  return($result);
}


// reads file fil from zip archive z and returns its content, or false on error
function read_file_from_zip($z, $fil) {
  $zip = new ZipArchive;
  if ($zip->open($z, ZipArchive::RDONLY) !== true) {
    echo "ERROR: failed to open zip file '{$z}'\n";
    return(false);
  }

  // load the appinfo/pkgname.lsm file
  $res = $zip->getFromName($fil, 8192, ZipArchive::FL_NOCASE);

  $zip->close();
  return($res);
}


// reads a LSM string and returns it in the form of an array
function parse_lsm($s) {
  $res = array();
  for ($l = strtok($s, "\n"); $l !== false; $l = strtok("\n")) {
    // the line is "token: value", let's find the colon
    $colpos = strpos($l, ':');
    if (($colpos === false) || ($colpos === 0)) continue;
    $tok = strtolower(trim(substr($l, 0, $colpos)));
    $val = trim(substr($l, $colpos + 1));
    $res[$tok] = $val;
  }
  return($res);
}


// ***************** MAIN ROUTINE *********************************************

echo "SvarDOS repository index generator ver {$PVER}\n";

if (($_SERVER['argc'] != 2) || ($_SERVER['argv'][1][0] == '-')) {
  echo "usage: php buildidx.php repodir\n";
  exit(1);
}

$repodir = $_SERVER['argv'][1];

echo "building index for directory {$repodir}...\n";

$pkgfiles = scandir($repodir);
$pkglist = '';
$pkgcount = 0;

// iterate over each zip file
foreach ($pkgfiles as $zipfile) {
  if (!preg_match('/.zip$/i', $zipfile)) continue; // skip non-zip files
  if (strchr($zipfile, '-')) {
    echo "skipping: {$zipfile}\n";
    continue; // skip alt vers (like dosmid-0.9.2.zip)
  }

  $path_parts = pathinfo($zipfile);
  $pkg = strtolower($path_parts['filename']);

  $zipfile_fullpath = realpath($repodir . '/' . $zipfile);

  $lsm = read_file_from_zip($zipfile_fullpath, "appinfo/{$pkg}.lsm");
  if ($lsm == false) {
    echo "ERROR: pkg {$z} does not contain an LSM file at the expected location\n";
    exit(1);
  }

  $lsmarray = parse_lsm($lsm);
  if (empty($lsmarray['version'])) {
    echo "ERROR: lsm file in {$zipfile} does not contain a version\n";
    var_dump($lsmarray);
    exit(1);
  }
  if (empty($lsmarray['description'])) {
    echo "ERROR: lsm file in {$zipfile} does not contain a description\n";
    exit(1);
  }

  $pkglist .= "{$pkg}\t{$lsmarray['version']}\t{$lsmarray['description']}\t" . file2bsum($zipfile_fullpath) . "\n";
  $pkgcount++;

}

echo "DONE - processed " . $pkgcount . " zip files\n";

file_put_contents($repodir . '/index.tsv', $pkglist);

exit(0);

?>

<?php /*

  SvarDOS repo index builder
  Copyright (C) Mateusz Viste 2012-2022

  buildidx computes an index json file for the SvarDOS repository.
  it must be executed pointing to a directory that stores packages (*.svp)
  files. buildidx will generate the index file and save it into the package
  repository.

  requires php-zip

  17 feb 2022: checking for non-8+3 filenames in packages and duplicates + devload no longer part of CORE
  16 feb 2022: added warning about overlong version strings and wild files location
  15 feb 2022: index is generated as json, contains all filenames and alt versions
  14 feb 2022: packages are expected to have the *.svp extension
  12 feb 2022: skip source packages from being processed (*.src.zip)
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

$PVER = "20220216";


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


function read_list_of_files_in_zip($z) {
  $zip = new ZipArchive;
  if ($zip->open($z, ZipArchive::RDONLY) !== true) {
    echo "ERROR: failed to open zip file '{$z}'\n";
    return(false);
  }

  $res = array();
  for ($i = 0; $i < $zip->numFiles; $i++) $res[] = $zip->getNameIndex($i);

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


// on PHP 8+ there is str_starts_with(), but not on PHP 7 so I use this
function str_head_is($haystack, $needle) {
  return strpos($haystack, $needle) === 0;
}


// ***************** MAIN ROUTINE *********************************************

//echo "SvarDOS repository index generator ver {$PVER}\n";

if (($_SERVER['argc'] != 2) || ($_SERVER['argv'][1][0] == '-')) {
  echo "usage: php buildidx.php repodir\n";
  exit(1);
}

$repodir = $_SERVER['argv'][1];

$pkgfiles = scandir($repodir);
$pkgcount = 0;


// load the list of CORE packages

$core_packages_list = explode(' ', 'amb attrib chkdsk choice command cpidos debug deltree diskcopy display dosfsck edit fc fdapm fdisk find format help himemx kernel keyb keyb_lay label localcfg mem mode more move pkg pkgnet shsucdx sort tree');


// do a list of all svp packages with their available versions and descriptions

$pkgdb = array();
foreach ($pkgfiles as $fname) {
  if (!preg_match('/.svp$/i', $fname)) continue; // skip non-svp files

  $path_parts = pathinfo($fname);
  $pkgnam = explode('-', $path_parts['filename'])[0];
  $pkgfullpath = realpath($repodir . '/' . $fname);

  $lsm = read_file_from_zip($pkgfullpath, "appinfo/{$pkgnam}.lsm");
  if ($lsm == false) {
    echo "ERROR: pkg {$fname} does not contain an LSM file at the expected location\n";
    continue;
  }
  $lsmarray = parse_lsm($lsm);
  if (empty($lsmarray['version'])) {
    echo "ERROR: lsm file in {$fname} does not contain a version\n";
    continue;
  }
  if (strlen($lsmarray['version']) > 16) {
    echo "ERROR: version string in lsm file of {$fname} is too long (16 chars max)\n";
    continue;
  }
  if (empty($lsmarray['description'])) {
    echo "ERROR: lsm file in {$fname} does not contain a description\n";
    continue;
  }

  // validate the files present in the archive
  $listoffiles = read_list_of_files_in_zip($pkgfullpath);
  $pkgdir = $pkgnam;

  // special rule for "parent and children" packages
  if (str_head_is($pkgnam, 'djgpp_')) $pkgdir = 'djgpp'; // djgpp_* packages put their files in djgpp
  if ($pkgnam == 'fbc_help') $pkgdir = 'fbc'; // FreeBASIC help goes to the FreeBASIC dir

  // array used to detect duplicated entries after lower-case conversion
  $duparr = array();

  foreach ($listoffiles as $f) {
    $f = strtolower($f);
    $path_array = explode('/', $f);
    // emit a warning when non-8+3 filenames are spotted and find duplicates
    foreach ($path_array as $item) {
      if (empty($item)) continue; // skip empty items at end of paths (eg. appinfo/)
      if (!preg_match("/[a-z0-9!#$%&'()@^_`{}~-]{1,8}(\.[a-z0-9!#$%&'()@^_`{}~-]{1,3}){0,1}/", $item)) {
        echo "WARNING: {$fname} contains a non-8+3 path (or weird char): {$item} (in $f)\n";
      }
    }
    // look for dups
    if (array_search($f, $duparr) !== false) {
      echo "WARNING: {$fname} contains a duplicated entry: '{$f}'\n";
    } else {
      $duparr[] = $f;
    }
    // LSM file is ok
    if ($f === "appinfo/{$pkgnam}.lsm") continue;
    if ($f === "appinfo/") continue;
    // CORE packages are premium citizens and can do a little more
    if (array_search($pkgnam, $core_packages_list) !== false) {
      if (str_head_is($f, 'bin/')) continue;
      if (str_head_is($f, 'cpi/')) continue;
      if (str_head_is($f, "doc/{$pkgdir}/")) continue;
      if ($f === 'doc/') continue;
      if (str_head_is($f, "nls/{$pkgdir}.")) continue;
      if ($f === 'nls/') continue;
    }
    // well-known "category" dirs are okay
    if (str_head_is($f, "progs/{$pkgdir}/")) continue;
    if ($f === 'progs/') continue;
    if (str_head_is($f, "devel/{$pkgdir}/")) continue;
    if ($f === 'devel/') continue;
    if (str_head_is($f, "games/{$pkgdir}/")) continue;
    if ($f === 'games/') continue;
    if (str_head_is($f, "drivers/{$pkgdir}/")) continue;
    if ($f === 'drivers/') continue;
    echo "WARNING: {$fname} contains a file in an illegal location: {$f}\n";
  }

  $meta['fname'] = $fname;
  $meta['desc'] = $lsmarray['description'];

  $pkgdb[$pkgnam][$lsmarray['version']] = $meta;
}

$db = array();

// iterate over each svp package
foreach ($pkgdb as $pkg => $versions) {

  // sort filenames by version, highest first
  uksort($versions, "version_compare");
  $versions = array_reverse($versions, true);

  foreach ($versions as $ver => $meta) {
    $fname = $meta['fname'];
    $desc = $meta['desc'];

    $bsum = file2bsum(realpath($repodir . '/' . $fname));

    $meta2['ver'] = strval($ver);
    $meta2['bsum'] = $bsum;

    if (empty($db[$pkg]['desc'])) $db[$pkg]['desc'] = $desc;
    $db[$pkg]['versions'][$fname] = $meta2;
  }

  $pkgcount++;

}

if ($pkgcount < 100) echo "WARNING: an unexpectedly low number of packages has been found in the repo ({$pkgcount})\n";

file_put_contents($repodir . '/_index.json', json_encode($db));

exit(0);

?>

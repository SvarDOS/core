<?php /*

  SvarDOS repo index builder
  Copyright (C) Mateusz Viste 2012-2024

  buildidx computes an index json file for the SvarDOS repository.
  it must be executed pointing to a directory that stores packages (*.svp)
  files. buildidx will generate the index file and save it into the package
  repository.

  requires php-zip

  20 may 2024: directory for alternative kernels changed to "KERNEL"
  10 mar 2024: support for "url" in LSM files
  01 feb 2024: computes the "latest" collection of symlinks
  24 nov 2023: SVED included in the MS-DOS compat list instead of EDIT + support for "release xyz" versions
  25 aug 2023: validation of the hwreq section in LSM files
  24 aug 2023: load hwreq data from LSM and store them in the json index + skip the '.svn' dir
  30 jun 2023: adapted for new CORE packages location (../packages-core)
  28 feb 2022: svarcom allowed to have a COMMAND.COM file without subdirectory
  24 feb 2022: added hardcoded hack to translate version 'x.xx' to '0.44' (NESticle)
  23 feb 2022: basic validation of source archives (not empty + matches an existing svp file)
  21 feb 2022: buildidx collects categories looking at the dir layout of each package + improved version string parsing (replaced version_compare call by dos_version_compare)
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

$PVER = "20240520";


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


// translates a version string into a array of integer values.
// Accepted formats follow:
//    300.12.1
//    1
//    12.2.34.2-4.5
//    1.2c
//    1.01 beta+3
//    2013-12-31
//    20220222 alpha
function vertoarr($verstr) {
  $subver = array(0,0,0,0);

  // switch string to lcase for easier processing and trim any leading or trailing white spaces
  $verstr = strtolower(trim($verstr));

  // Special hack for E. C. Masloch's lDebug. lDebug's versions are identifying as "releases" and wish to be recognized as such. If the version string starts with "release " I remove this word and continue.
  if (preg_match('/^release /', $verstr)) $verstr = substr($verstr, 8);

  // replace all '-' and '/' characters to '.' (uniformization of sub-version parts delimiters)
  $verstr = strtr($verstr, '-/', '..');

  // is there a subversion value? (for example "+4" in "1.05+4")
  $i = strrpos($verstr, '+', 1);
  if ($i !== false) {
    // validate the svar-version is a proper integer
    $svarver = substr($verstr, $i + 1);
    if (! preg_match('/[1-9][0-9]*/', $svarver)) {
      return(false);
    }
    $subver[3] = intval($svarver); // set the +rev as a very minor item
    $verstr = substr($verstr, 0, $i);
  }

  // NESticls hack: version "x.xx" is translated to "0.44"... that sucks but that's how it is.
  // ref: https://web.archive.org/web/20070205074631/http://www.zophar.net/NESticle/
  if ($verstr == 'x.xx') $verstr = '0.44';

  // beta reordering: convert "beta 0.95" to "0.95 beta"
  if (preg_match('/^beta /', $verstr)) $verstr = substr($verstr, 5) . ' beta';

  // any occurence of alpha,beta,gamma,delta etc preceded by a digit should have a space separator added
  // example: "2.6.0pre9" becomes "2.6.0 pre9"
  $verstr = preg_replace('/([0-9])(alpha|beta|gamma|delta|pre|rc|patch)/', '$1 $2', $verstr);

  // same as above, but this time adding a trailing space separator
  // example: "2.6.0 pre9" becomes "2.6.0 pre 9"
  $verstr = preg_replace('/(alpha|beta|gamma|delta|pre|rc|patch)([0-9])/', '$1 $2', $verstr);

  // is the version ending with ' alpha', 'beta', etc?
  if (preg_match('/ (alpha|beta|gamma|delta|pre|rc|patch)( [0-9]{1,4}){0,1}$/', $verstr)) {
    // if there is a trailing beta-number, process it first
    if (preg_match('/ [0-9]{1,4}$/', $verstr)) {
      $i = strrpos($verstr, ' ');
      $subver[2] = intval(substr($verstr, $i + 1));
      $verstr = trim(substr($verstr, 0, $i));
    }
    $i = strrpos($verstr, ' ');
    $greek = substr($verstr, $i + 1);
    $verstr = trim(substr($verstr, 0, $i));
    if ($greek == 'alpha') {
      $subver[1] = 1;
    } else if ($greek == 'beta') {
      $subver[1] = 2;
    } else if ($greek == 'gamma') {
      $subver[1] = 3;
    } else if ($greek == 'delta') {
      $subver[1] = 4;
    } else if ($greek == 'pre') {
      $subver[1] = 5;
    } else if ($greek == 'rc') {
      $subver[1] = 6;
    } else if ($greek == 'patch') { // this is a POST-release version, as opposed to all above that are PRE-release versions
      $subver[1] = 99;
    } else {
      return(false);
    }
  } else {
    $subver[1] = 98; // one less than the 'patch' level
  }

  // does the version string have a single-letter subversion? (1.0c)
  if (preg_match('/[a-z]$/', $verstr)) {
    $subver[0] = ord(substr($verstr, -1));
    $verstr = substr_replace($verstr, '', -1); // remove last character from string
  }

  // convert "30-jan-99", "1999-jan-30" and "30-jan-1999" versions to "30jan99" or "30jan1999"
  // note that dashes have already been replaced by dots
  if (preg_match('/^([0-9][0-9]){1,2}\.(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)\.([0-9][0-9]){1,2}$/', $verstr)) {
    $verstr = str_replace('.', '', $verstr);
  }

  // convert "2009mar17" versions to "17mar2009"
  if (preg_match('/^[0-9]{4}(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[0-9]{2}$/', $verstr)) {
    $dy = substr($verstr, 7);
    $mo = substr($verstr, 4, 3);
    $ye = substr($verstr, 0, 4);
    $verstr = "{$dy}{$mo}{$ye}";
  }

  // convert "30jan99" versions to 99.1.30 and "30jan1999" to 1999.1.30
  if (preg_match('/^[0-3][0-9](jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)([0-9][0-9]){1,2}$/', $verstr)) {
    $months = array('jan' => 1, 'feb' => 2, 'mar' => 3, 'apr' => 4, 'may' => 5, 'jun' => 6, 'jul' => 7, 'aug' => 8, 'sep' => 9, 'oct' => 10, 'nov' => 11, 'dec' => 12);
    $dy = substr($verstr, 0, 2);
    $mo = $months[substr($verstr, 2, 3)];
    $ye = substr($verstr, 5);
    $verstr = "{$ye}.{$mo}.{$dy}";
  }

  // validate the format is supported, should be something no more complex than 1.05.3.33
  if (! preg_match('/^[0-9][0-9.]{0,20}$/', $verstr)) {
    return(false);
  }

  // NOTE: a zero right after a separator and trailed with a digit (as in 1.01)
  //       has a special meaning
  $exploded = explode('.', $verstr);
  if (count($exploded) > 16) {
    return(false);
  }
  $exploded[16] = $subver[0]; // a-z (1.0c)
  $exploded[17] = $subver[1]; // alpha/beta/gamma/delta/rc/pre
  $exploded[18] = $subver[2]; // alpha-beta-gamma subversion (eg. "beta 9")
  $exploded[19] = $subver[3]; // svar-ver (1.0+5)
  for ($i = 0; $i < 20; $i++) if (empty($exploded[$i])) $exploded[$i] = '0';

  ksort($exploded);

  return($exploded);
}


function dos_version_compare($v1, $v2) {
  $v1arr = vertoarr($v1);
  $v2arr = vertoarr($v2);
  for ($i = 0; $i < count($v1arr); $i++) {
    if ($v1arr[$i] > $v2arr[$i]) return(1);
    if ($v1arr[$i] < $v2arr[$i]) return(-1);
  }
  return(0);
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


// returns an array that contains CORE packages (populated from the core subdirectory in pkgdir)
function load_core_list($repodir_core) {
  $res = array();

  foreach (scandir($repodir_core) as $f) {
    if (!preg_match('/\.svp$/', $f)) continue;
    $res[] = explode('.', $f)[0];
  }
  return($res);
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


// load the list of CORE and MSDOS_COMPAT packages

$core_packages_list = load_core_list($repodir . '/../packages-core/');
$msdos_compat_list = explode(' ', 'append assign attrib callver chkdsk choice comp cpidos debug defrag deltree diskcomp diskcopy display edlin exe2bin fc fdapm fdisk find format help himemx kernel keyb label localcfg mem mirror mode more move nlsfunc print replace share shsucdx sort svarcom sved swsubst tree undelete unformat xcopy');

// do a list of all svp packages with their available versions and descriptions

$pkgdb = array();
foreach ($pkgfiles as $fname) {

  // zip files (ie. source archives)
  if (preg_match('/\.zip$/', $fname)) {
    // the zip archive should contain at least one file
    if (count(read_list_of_files_in_zip($repodir . '/' . $fname)) < 1) echo "WARNING: source archive {$fname} contains no files (either empty or corrupted)\n";
    // check that the file relates to an existing svp package
    $svpfname = preg_replace('/zip$/', 'svp', $fname);
    if (!file_exists($repodir . '/' . $svpfname)) echo "ERROR: orphaned source archive '{$fname}' (no matching svp file, expecting a package named '{$svpfname}')\n";
    // that is for zip files
    continue;
  }

  // silently skip the hidden .svn directory
  if ($fname === '.svn') continue;

  // skip (and warn about) non-svp
  if (!preg_match('/\.svp$/', $fname)) {
    $okfiles = array('.', '..', '_cats.json', '_index.json', '_buildidx.log');
    if (array_search($fname, $okfiles) !== false) continue;
    echo "WARNING: wild file '{$fname}' (this is either an useless file that should be removed, or a misnamed package or source archive)'\n";
    continue;
  }

  if (!preg_match('/^[a-zA-Z0-9+. _-]*\.svp$/', $fname)) {
    echo "ERROR: {$fname} has a very weird name\n";
    continue;
  }

  $path_parts = pathinfo($fname);
  $pkgnam = explode('-', $path_parts['filename'])[0];
  $pkgfullpath = realpath($repodir . '/' . $fname);

  $lsm = read_file_from_zip($pkgfullpath, "appinfo/{$pkgnam}.lsm");
  if ($lsm == false) {
    echo "ERROR: {$fname} does not contain an LSM file at the expected location\n";
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
  if ($pkgnam == 'clamdb') $pkgdir = 'clamav'; // data patterns for clamav

  // array used to detect duplicated entries after lower-case conversion
  $duparr = array();

  // will hold the list of categories that this package belongs to
  $catlist = array();

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
    // CORE and MSDOS_COMPAT packages are premium citizens and can do a little more
    $core_or_msdoscompat = 0;
    if (array_search($pkgnam, $core_packages_list) !== false) {
      $catlist[] = 'core';
      $core_or_msdoscompat = 1;
    }
    if (array_search($pkgnam, $msdos_compat_list) !== false) {
      $catlist[] = 'msdos_compat';
      $core_or_msdoscompat = 1;
    }
    if ($core_or_msdoscompat == 1) {
      if (str_head_is($f, 'bin/')) continue;
      if (str_head_is($f, 'cpi/')) continue;
      if (str_head_is($f, "doc/{$pkgdir}/")) continue;
      if ($f === 'doc/') continue;
      if (str_head_is($f, "nls/{$pkgdir}.")) continue;
      if ($f === 'nls/') continue;
    }
    // SVARCOM is allowed to have a root-based COMMAND.COM file
    if ($pkgnam === 'svarcom') {
      if ($f === 'command.com') continue;
    }
    // the help package is allowed to put files in... help
    if (($pkgnam == 'help') && (str_head_is($f, 'help/'))) continue;
    // must be category-prefixed file, add it to the list of categories for this package
    $catlist[] = explode('/', $f)[0];
    // well-known "category" dirs are okay
    if (str_head_is($f, "progs/{$pkgdir}/")) continue;
    if ($f === 'progs/') continue;
    if (str_head_is($f, "devel/{$pkgdir}/")) continue;
    if ($f === 'devel/') continue;
    if (str_head_is($f, "games/{$pkgdir}/")) continue;
    if ($f === 'games/') continue;
    if (str_head_is($f, "drivers/{$pkgdir}/")) continue;
    if ($f === 'drivers/') continue;
    if (str_head_is($f, "kernel/{$pkgdir}/")) continue;
    echo "WARNING: {$fname} contains a file in an illegal location: {$f}\n";
  }

  // do I understand the version string?
  if (vertoarr($lsmarray['version']) === false) echo "WARNING: {$fname} parsing of version string failed ('{$lsmarray['version']}')\n";

  $meta = array();
  $meta['fname'] = $fname;
  $meta['desc'] = $lsmarray['description'];
  $meta['cats'] = array_unique($catlist);
  $meta['url'] = $lsmarray['url'];

  if (!empty($lsmarray['hwreq'])) {
    $meta['hwreq'] = explode(' ', strtolower($lsmarray['hwreq']));
    sort($meta['hwreq']);

    // validate list of valid hwreq tokens
    $validtokens = array('8086', '186', '286', '386', '486', '586', 'fpu', 'mda', 'cga', 'ega', 'vga', 'mcga', 'svga');
    foreach (array_diff($meta['hwreq'], $validtokens) as $tok) echo "WARNING: {$fname} contains an LSM hwreq section with invalid token: {$tok}\n";
  }

  $pkgdb[$pkgnam][$lsmarray['version']] = $meta;
}


$db = array();
$cats = array();

// ******** compute the version-sorted list of packages with a single *********
// ******** description and category list for each package ********************

// iterate over each svp package
foreach ($pkgdb as $pkg => $versions) {

  // sort filenames by version, highest first
  uksort($versions, "dos_version_compare");
  $versions = array_reverse($versions, true);

  foreach ($versions as $ver => $meta) {
    $fname = $meta['fname'];
    $desc = $meta['desc'];
    $url = $meta['url'];

    $bsum = file2bsum(realpath($repodir . '/' . $fname));

    $meta2 = array();
    $meta2['ver'] = strval($ver);
    $meta2['bsum'] = $bsum;
    if (!empty($meta['hwreq'])) $meta2['hwreq'] = $meta['hwreq'];

    if (empty($db[$pkg]['desc'])) $db[$pkg]['desc'] = $desc;
    if (empty($db[$pkg]['url'])) $db[$pkg]['url'] = $url;
    if (empty($db[$pkg]['cats'])) {
      $db[$pkg]['cats'] = $meta['cats'];
      $cats = array_unique(array_merge($cats, $meta['cats']));
    }
    $db[$pkg]['versions'][$fname] = $meta2;
  }

  $pkgcount++;

}

if ($pkgcount < 100) echo "WARNING: an unexpectedly low number of packages has been found in the repo ({$pkgcount})\n";

$json_blob = json_encode($db);
if ($json_blob === false) {
  echo "ERROR: JSON convertion failed! -> ";
  switch (json_last_error()) {
    case JSON_ERROR_DEPTH:
      echo 'maximum stack depth exceeded';
      break;
    case JSON_ERROR_STATE_MISMATCH:
      echo 'underflow of the modes mismatch';
      break;
    case JSON_ERROR_CTRL_CHAR:
      echo 'unexpected control character found';
      break;
    case JSON_ERROR_UTF8:
      echo 'malformed utf-8 characters';
      break;
    default:
      echo "unknown error";
      break;
  }
  echo "\n";
}

file_put_contents($repodir . '/_index.json', $json_blob);

$cats_json = json_encode($cats);
file_put_contents($repodir . '/_cats.json', $cats_json);


// populate the 'latest' dir with symlinks to latest version of every svp
mkdir($repodir . '/latest');
foreach ($db as $pkg => $props) {
  $fname = array_key_first($props['versions']);
  $cat = array_values($props['cats'])[0];
  //echo "pkg = '{$pkg}' ; fname = '{$fname}' ; cat = '{$cat}'\n";
  if (!file_exists($repodir . '/latest/' . $cat)) mkdir($repodir . '/latest/' . $cat);
  symlink('../../' . $fname, $repodir . '/latest/' . $cat . '/' . $pkg . '.svp');
}


exit(0);

?>

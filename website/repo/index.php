<?php

/*
  pkgnet interface
  Copyright (C) 2021-2022 Mateusz Viste

 === API ===
  ?a=pull&p=PACKAGE[-VER]     downloads the svp archive of PACKAGE (possibly of VER version)
  ?a=pullsrc&p=PACKAGE[-VER]  downloads the source zip of PACKAGE (possibly of VER version)
  ?a=search&p=PHRASE[::cat]   list packages that match PHRASE (possibly filtered by cat category)
  ?a=checkup                  list of packages+versions in $_POST
*/


// messages in different languages (and necessary mappings for codepage conversions)
include 'lang.php';


// convert a utf-8 string into codepage related to lang
function cp_conv($s, $lang) {
  global $CP_UTF, $CP_ENC;
  $res = str_replace($CP_UTF[$lang], $CP_ENC[$lang], $s);
  return($res);
}


function get_msg($id, $lang) {
  global $MSG;
  if (!empty($MSG[$id][$lang])) {
    return cp_conv($MSG[$id][$lang], $lang);
  } else {
    return $MSG[$id]['en'];
  }
}


function nicesize($bytes) {
  if ($bytes < 1024) return(round($bytes / 1024, 1) . "K");
  if ($bytes < 1024 * 1024) return(round($bytes / 1024) . "K");
  return(round($bytes / 1024 / 1024, 1) . "M");
}


function csv_to_array($filename = '', $delimiter = "\t") {
  //if (! file_exists($filename) || ! is_readable($filename)) return FALSE;
  $handle = fopen($filename, 'r');
  if ($handle === false) return(false);

  $rez = array();

  while (($row = fgetcsv($handle, 1000, $delimiter)) !== FALSE) {
    $rez[] = $row;
  }
  fclose($handle);

  return $rez;
}


function tabulprint($ar_data, $ar_width, $margin = true) {
  $count = 0;
  foreach ($ar_data as $item) {
    if ($count == 0) {
      echo '|';
      if ($margin) echo ' ';
    }
    echo substr(str_pad($item, $ar_width[$count]), 0, $ar_width[$count]);
    if ($margin) {
      echo ' | ';
    } else {
      echo '|';
    }
    $count++;
  }
  echo "\r\n";
}


// *** MAIN START ************************************************************


if (empty($_GET['a'])) {
  http_response_code(404);
  echo "ERROR: no action specified\r\n";
  exit(0);
}

$lang = 'en';
if ((!empty($_GET['lang'])) && (preg_match('/[a-zA-Z][a-zA-Z]/', $_GET['lang']))) $lang = strtolower($_GET['lang']);

// load pkg desc translations
$descdb = json_decode(file_get_contents("pkg_desc_{$lang}.json"), true);

// switch to the packages directory
if (chdir('../../packages') === false) {
  http_response_code(404);
  echo "ERROR: server-side error, cannot access packages\r\n";
  exit(0);
}

$a = strtolower($_GET['a']);

$p = '';
$v = '';
if ($a != 'checkup') {
  if (empty($_GET['p'])) {
    http_response_code(404);
    echo "ERROR: no package specified\r\n";
    exit(0);
  }
  // pull and pullsrc actions accept pkg-ver strings
  if ($a == 'pull' || $a == 'pullsrc') {
    $pv = explode('-', strtolower($_GET['p']));
    $p = $pv[0];
    if (!empty($pv[1])) $v = $pv[1];
  } else {
    $p = strtolower($_GET['p']);
  }
}


// is action valid?

if (($a !== 'search') && ($a !== 'checkup') && ($a !== 'pull') && ($a !== 'pullsrc')) {
  http_response_code(404);
  echo "ERROR: invalid action\r\n";
  exit(0);
}


// load pkg db

$db = json_decode(file_get_contents('_index.json'), true);
if (empty($db)) {
  http_response_code(404);
  echo "ERROR: server error, database not found\n";
  exit(0);
}


// pull or pullsrc action (svp / zip)

if (($a === 'pull') || ($a === 'pullsrc')) {
  $fname = false;
  if (empty($v)) { // take first version (that's the preferred one)
    $fname = array_key_first($db[$p]['versions']);
  } else {
    // look for a specific version string
    foreach ($db[$p]['versions'] as $f => $e) {
      if (strcasecmp($e['ver'], $v) == 0) {
        $fname = $f;
        break;
      }
    }
  }
  if (file_exists($fname)) {
    if ($a === 'pullsrc') { // is it about source?
      $fname = preg_replace('/svp$/', 'zip', $fname); // replace *.svp by *.zip
      if (file_exists($fname)) {
        header('Content-Disposition: attachment; filename="' . $fname);
        header('Content-Type: application/octet-stream');
        readfile($fname);
      } else {
        http_response_code(404);
        echo get_msg('PKG_NO_SRC', $lang) . "\r\n";
      }
    } else {
      header('Content-Disposition: attachment; filename="' . $fname);
      header('Content-Type: application/octet-stream');
      readfile($fname);
    }
  } else {
    http_response_code(404);
    echo get_msg('PKG_NOT_FOUND', $lang) . "\r\n";
  }
  exit(0);
}


// search action

if ($a === 'search') {
  $matches = 0;
  header('Content-Type: text/plain');

  // if catfilter present, trim it out of the search term
  $exp_cat = explode('::', $p);
  $p = $exp_cat[0];
  $catfilter = '';
  if (!empty($exp_cat[1])) $catfilter = strtolower($exp_cat[1]);

  foreach ($db as $pkg => $meta) {
    // apply the category filter, if any
    if (! empty($catfilter)) {
      if (array_search($catfilter, $meta['cats']) === false) continue;
    }
    // look for term
    if ((empty($p)) || (stristr($pkg, $p)) || (stristr($meta['desc'], $p))) {
      // fetch first (preferred) version
      $prefver_fname = array_key_first($meta['versions']);
      $prefver = array_shift($meta['versions']);
      echo str_pad(strtoupper($pkg), 12);
      echo str_pad(get_msg('VER', $lang) . " {$prefver['ver']} ", 16);
      echo str_pad(get_msg('SIZE', $lang) . ' ' . nicesize(filesize($prefver_fname)), 16) . "BSUM: " . sprintf("%04X", $prefver['bsum']) . "\r\n";

      // do I have a localized version of the description?
      if (!empty($descdb[$pkg])) {
        echo wordwrap(cp_conv($descdb[$pkg], $lang), 79, "\r\n", true);
      } else {
        echo wordwrap($meta['desc'], 79, "\r\n", true);
      }

      echo "\r\n";
      // do I have any alt versions?
      $altlist = array();
      foreach ($meta['versions'] as $altver) {
        $altlist[] = $pkg . '-' . $altver['ver'];
      }
      if (!empty($altlist)) {
        echo wordwrap('[' . get_msg('ALT_VERS', $lang) . ' ' . implode(', ', $altlist), 79, "\r\n", true) . "]\r\n";
      }
      echo "\r\n";
      $matches++;
    }
  }
  if ($matches == 0) echo get_msg('NO_MATCHING_PKG', $lang) . "\r\n";
}


// checkup action

if ($a === 'checkup') {
  $found = 0;
  $remote_pkgs = csv_to_array("php://input", "\t"); // [0] = pkgname ; [1] = version

  foreach ($remote_pkgs as $rpkg) {
    $rpkg[0] = strtolower($rpkg[0]);
    if (empty($db[$rpkg[0]])) continue;

    $dbpkg = $db[$rpkg[0]];
    // compare user's version with preferred version in repo
    $prefver = array_shift($dbpkg['versions']);
    if (strcasecmp($prefver['ver'], $rpkg[1]) == 0) continue;
    // found version mismatch
    if ($found == 0) {
      echo str_pad('', 78, '-') . "\r\n";
      tabulprint(array(get_msg('PACKAGE', $lang), get_msg('INSTALLED', $lang), get_msg('AVAILABLE', $lang)), array(8, 30, 30));
      tabulprint(array('----------', '--------------------------------', '--------------------------------'), array(10, 32, 32), false);
    }
    $found++;
    tabulprint(array('' . $rpkg[0], $rpkg[1], $prefver['ver']), array(8, 30, 30));
  }
  if ($found == 0) {
    echo get_msg('NO_UPDATES', $lang) . "\r\n";
  } else {
    echo str_pad('', 78, '-') . "\r\n";
    echo get_msg('FOUND_DIFFER', $lang) . ' ' . $found . "\r\n";
  }
}

?>

<?php

/*
  pkgnet interface
  Copyright (C) 2021-2022 Mateusz Viste

 === API ===
  ?a=pull&p=PACKAGE           downloads the zip archive (svp) containing PACKAGE
  ?a=pull&p=PACKAGE-VER       downloads the zip (svp) containing PACKAGE in version VER
  ?a=search&p=PHRASE          list packages that match PHRASE
  ?a=checkup                  list of packages+versions in $_POST
*/


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
  $pv = explode('-', strtolower($_GET['p']));
  $p = $pv[0];
  if (!empty($pv[1])) $v = $pv[1];
}


// is action valid?

if (($a !== 'search') && ($a !== 'checkup') && ($a !== 'pull')) {
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


// pull action

if ($a === 'pull') {
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
    header('Content-Disposition: attachment; filename="' . $fname);
    header('Content-Type: application/octet-stream');
    readfile($fname);
  } else {
    http_response_code(404);
    echo "ERROR: package not found on server\r\n";
  }
  exit(0);
}


// search action

if ($a === 'search') {
  $matches = 0;
  foreach ($db as $pkg => $meta) {
    if ((stristr($pkg, $p)) || (stristr($meta['desc'], $p))) {
      // fetch first (preferred) version
      $prefver_fname = array_key_first($meta['versions']);
      $prefver = array_shift($meta['versions']);
      echo str_pad(strtoupper($pkg), 12) . str_pad("ver: {$prefver['ver']} ", 16) . str_pad("size: " . nicesize(filesize($prefver_fname)), 16) . "BSUM: " . sprintf("%04X", $prefver['bsum']) . "\r\n";
      echo wordwrap($meta['desc'], 79, "\r\n", true);
      echo "\r\n";
      // do I have any alt versions?
      $altlist = array();
      foreach ($meta['versions'] as $altver) {
        $altlist[] = $pkg . '-' . $altver['ver'];
      }
      if (!empty($altlist)) {
        echo wordwrap("[alt versions: " . implode(', ', $altlist), 79, "\r\n", true) . "]\r\n";
      }
      echo "\r\n";
      $matches++;
    }
  }
  if ($matches == 0) echo "No matching package found.\r\n";
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
      echo str_pad('', 58, '-') . "\r\n";
      tabulprint(array('PACKAGE', 'INSTALLED (LOCAL)', 'AVAILABLE (REMOTE)'), array(8, 20, 20));
      tabulprint(array('----------', '----------------------', '----------------------'), array(10, 22, 22), false);
    }
    $found++;
    tabulprint(array('' . $rpkg[0], $rpkg[1], $prefver['ver']), array(8, 20, 20));
  }
  if ($found == 0) {
    echo "no available updates\r\n";
  } else {
    echo str_pad('', 58, '-') . "\r\n";
    echo "found {$found} differing package(s)\r\n";
  }
}

?>

<?php

/*
  pkgnet interface
  Copyright (C) 2021 Mateusz Viste

 === API ===
  ?a=pull&p=PACKAGE           downloads the zip archive containing PACKAGE
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

$a = strtolower($_GET['a']);

$p = '';
if ($a != 'checkup') {
  if (empty($_GET['p'])) {
    http_response_code(404);
    echo "ERROR: no package specified\r\n";
    exit(0);
  }
  $p = strtolower($_GET['p']);
}

$v = '';
if (!empty($_GET['v'])) $v = $_GET['v'];

// pull action is easy (does not require looking into pkg list), do it now

if ($a === 'pull') {
  if (file_exists($p . '.zip')) {
    readfile($p . '.zip');
  } else {
    http_response_code(404);
    echo "ERROR: package not found on server\r\n";
  }
  exit(0);
}

// is action valid?

if (($a !== 'search') && ($a !== 'checkup')) {
  http_response_code(404);
  echo "ERROR: invalid action\r\n";
  exit(0);
}

// iterate over packages now

$handle = fopen("index.tsv", "rb");
if ($handle === FALSE) {
  http_response_code(404);
  echo "ERROR: Server-side internal error\r\n";
  exit(0);
}

if ($a === 'search') {
  $matches = 0;
  while (($pkg = fgetcsv($handle, 1024, "\t")) !== FALSE) {
    if ((stristr($pkg[0], $p)) || (stristr($pkg[2], $p))) {
      echo str_pad(strtoupper($pkg[0]), 12) . str_pad("ver: {$pkg[1]}", 13) . str_pad("size: " . nicesize(filesize($pkg[0] . '.zip')), 13) . "BSUM: " . sprintf("%04X", $pkg[3]) . "\r\n";
      echo wordwrap($pkg[2], 79, "\r\n", true);
      echo "\r\n\r\n";
      $matches++;
    }
  }
  if ($matches == 0) echo "No matching package found.\r\n";
}

if ($a === 'checkup') {
  $found = 0;
  $remote_pkgs = csv_to_array("php://input", "\t"); // [0] = pkgname ; [1] = version
  while (($pkg = fgetcsv($handle, 1024, "\t")) !== FALSE) {
    // is $pkg part of remote packages?
    foreach ($remote_pkgs as $rpkg) {
      if (strcasecmp($pkg[0], $rpkg[0]) != 0) continue;
      if ($pkg[1] === $rpkg[1]) continue; // skip same version
      if ($found == 0) {
        echo str_pad('', 58, '-') . "\r\n";
        tabulprint(array('PACKAGE', 'INSTALLED (LOCAL)', 'AVAILABLE (REMOTE)'), array(8, 20, 20));
        tabulprint(array('----------', '----------------------', '----------------------'), array(10, 22, 22), false);
      }
      $found++;
      tabulprint(array('' . $pkg[0], $rpkg[1], $pkg[1]), array(8, 20, 20));
      break;
    }
  }
  if ($found == 0) {
    echo "no available updates\r\n";
  } else {
    echo str_pad('', 58, '-') . "\r\n";
    echo "found {$found} differing packages\r\n";
  }
}
fclose($handle);


?>

<?php

/*
  pkgnet interface
  Copyright (C) 2021 Mateusz Viste

 === API ===
  ?a=pull&p=PACKAGE           downloads the zip archive containing PACKAGE
  ?a=search&p=PHRASE          list packages that match PHRASE
  ?a=checkup&p=PACKAGE&v=ver  check if package available in version > v
*/


function nicesize($bytes) {
  if ($bytes < 1024) return(round($bytes / 1024, 1) . "K");
  if ($bytes < 1024 * 1024) return(round($bytes / 1024) . "K");
  return(round($bytes / 1024 / 1024, 1) . "M");
}


if (empty($_GET['a'])) {
  http_response_code(404);
  echo "ERROR: no action specified\r\n";
  exit(0);
}

if (empty($_GET['p'])) {
  http_response_code(404);
  echo "ERROR: no package specified\r\n";
  exit(0);
}

$a = strtolower($_GET['a']);
$p = strtolower($_GET['p']);

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
      echo str_pad(strtoupper($pkg[0]), 12) . str_pad("ver: {$pkg[1]}", 13) . str_pad("size: " . nicesize(filesize($pkg[0] . '.zip')), 13) . "BSUM: {$pkg[3]}\r\n";
      echo wordwrap($pkg[2], 79, "\r\n", true);
      echo "\r\n\r\n";
      $matches++;
    }
  }
  if ($matches == 0) echo "No matching package found.\r\n";
}

if ($a === 'checkup') {
  while (($pkg = fgetcsv($handle, 1024, "\t")) !== FALSE) {
    if (strcasecmp($pkg[0], $p)) {
      echo "found package {$pkg[0]} ver {$pkg[1]} -> is it newer than '{$v}' ?\r\n";
      break;
    }
  }
}
fclose($handle);


?>

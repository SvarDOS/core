<?php

/*
  pkgnet interface
  Copyright (C) 2021 Mateusz Viste

 === API ===
  ?a=pull&p=PACKAGE           downloads the zip archive containing PACKAGE
  ?a=search&p=PHRASE          list packages that match PHRASE
  ?a=checkup&p=PACKAGE&v=ver  check if package available in version > v
*/

if (empty($_GET['a']) {
  http_response_code(404);
  exit 0;
}

$a = $_GET['a'];
$p = '';
$v = '';

if (!empty($_GET['p'])) $p = $_GET['p'];
if (!empty($_GET['v'])) $v = $_GET['v'];

echo "I received a='{$a}' p='{$p}' v='{$v}'\n";

?>

<!DOCTYPE html>
<html lang="en">
  <head>
    <title>SvarDOS</title>
    <meta name="keywords" content="svardos,svarog386,freedos">
    <meta name="author" content="Mateusz Viste">
    <meta name="robots" content="index, follow">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="style.css">
  </head>
  <body>

    <p style="margin: 0 0 -1em auto; font-size: 0.9em; text-align: right;"><a href="/">Main page</a> I <a href="?p=repo">Packages</a> I <a href="?p=help">Help</a> I <a href="?p=forum">Forum</a>  <!-- I <a href="?p=nls">NLS</a>--></p>

<?php

    $p = '';
    if (! empty($_GET['p'])) $p = $_GET['p'];

    if ($p == 'nls') {
      readfile('index-nls.htm');
    } else if ($p == 'repo') {
      include 'index-repo.php';
    } else if ($p == 'help') {
      include 'index-help.php';
    } else if ($p == 'files') {
      include 'index-files.php';
    } else if ($p == 'forum') {
      include 'index-forum.php';
    } else { // else display the front page
      include 'index-main.php';
    }
?>
  </body>
</html>

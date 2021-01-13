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

    <p style="margin: 0 0 -1em auto; font-size: 0.9em; text-align: right;"><a href="/">Main page</a> I <a href="?p=why">Why SvarDOS</a> I <a href="?p=repo">Repo</a> I <a href="?p=tech">Tech</a> I <a href="?p=nls">NLS</a></p>

<?php

    $p = '';
    if (! empty($_GET['p'])) $p = $_GET['p'];

    if ($p == 'why') {
      readfile('index-why.htm');
    } else if ($p == 'tech') {
      include 'index-tech.php';
    } else if ($p == 'nls') {
      readfile('index-nls.htm');
    } else if ($p == 'repo') {
      include 'index-repo.php';
    } else if ($p == 'test') {
      include 'index-main.php';
    } else { // else display the front page
      include 'index-main.php';
    }
?>
  </body>
</html>

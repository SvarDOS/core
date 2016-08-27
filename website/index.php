<!DOCTYPE html>
<html lang="en">
  <head>
    <title>Svarog386 - a FreeDOS distribution for 386+</title>
    <meta name="keywords" content="svarog386,freedos">
    <meta name="author" content="Mateusz Viste">
    <meta name="robots" content="index, follow">
    <meta charset="UTF-8">
    <link rel="icon" type="image/png" href="icon.png">
    <link rel="stylesheet" href="style.css">
  </head>
  <body>

    <div style="background-color: #FAFAFA; width: 70%; min-width: 800px; margin: 0 auto; height: auto !important; min-height: 100%; box-shadow: 0 0 2em 0.25em #202020;"><div style="padding: 0.5em 0.5em 2em 0.5em;">

    <p style="margin: 0 0 -1em auto; font-size: 0.9em; text-align: right;"><a href="/">Main page</a> I <a href="?p=why">Why Svarog386</a> I <a href="?p=tech">Tech</a></p>

<?php

    $p = '';
    if (! empty($_GET['p'])) $p = $_GET['p'];

    if ($p == 'why') {
      readfile('index-why.htm');
    } else if ($p == 'tech') {
      include 'index-tech.php';
    } else if ($p == 'pkgincl') { // this one should be removed soon, once the tech version is discovered by search engines -- 26 Aug 2016
      echo '
      <p class="title">Svarog386 package inclusion rules</p>
      <p class="copyr"></p>
      <p>Svarog386 is a FreeDOS&trade; distribution that comes with plenty of third-party packages. With time, packages gets updated and new packages are being added. However, Svarog386 is not a shareware distribution CD, nor it is a "warez" production of any kind! Every software that is distributed within Svarog386 must comply to a few common sense rules, as listed below.</p>

      <p class="chapt">Objective usefulness</p>
      <p>The distributed software must be useful. There is no point in distributing hundreds of "Hello World" programs for example. The software must be useable as a finished product and provide some features that are proven to be seeked by at least a subset of the user base. Games are considered useful, as long as they fulfill their goal of providing actual distraction.</p>

      <p class="chapt">Reasonable quality</p>
      <p>The packaged program must exhibit traits of reasonable quality. This means that it should have a deterministic behavior, and be free of undesirable side-effects to the user\'s computer (not crashing, freezing, resulting in unexpected loss of data, etc). It should also provide clear, non-ambiguous instructions to the user about how the program is meant to be used.</p>

      <p class="chapt">Free (no cost)</p>
      <p>The program must be free - that is, available at no financial cost. It doesn\'t have to comply to an OSI-approved license or be open-source (even if that would be prefered), but at the very least it must be free for personal, non-commercial use.</p>

      <p class="chapt">Distribution allowed</p>
      <p>The program must allow distribution without restrictions, and must not forbid being redistributed in a re-packaged form.</p>
      ';
    } else { // else display the front page
      include 'index-main.php';
    }
?>
    </div></div>
  </body>
</html>

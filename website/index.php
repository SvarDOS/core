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
      echo '
      <p class="title">Svarog386 - a FreeDOS distribution for 386+ computers</p>
      <p class="copyr">brought to you by <a href="http://mateusz.viste.fr/" class="mateusz">Mateusz Viste</a></p>
      <img src="svarog386.png" alt="logo" style="height: 10em; float: right; margin: 0 0 0.5em 0.5em;">

      <p>Svarog386 is a DOS distribution based on FreeDOS. It\'s released in the form of a single bootable CD image (ISO) that contains the FreeDOS kernel, a command interpreter and a variety of third-party packages.<br>
      Svarog386 is a "rolling release", meaning that it doesn\'t follow the concept of "versions". Svarog386 can be kept up-to-date either via online update repositories or by using the latest ISO image as your local package repository.</p>

      <p>Svarog386 is <b>not</b> designed for strict 8086 compatibility. Many parts of it might require a 386-class CPU. If you look for a simple 8086 FreeDOS distribution, take a look at <a href="http://svarog86.sourceforge.net">Svarog86</a>.</p>

      <p>Need to get in touch, or wish to contribute some packages? Feel free to drop a line to the usenet group <b>alt.os.free-dos</b>. Alternatively, you could also reach me directly via <a href="https://sourceforge.net/u/userid-1220451/">sourceforge</a>.
      ';

      // find the latest ISO file available
      $files = glob("*.iso", GLOB_NOSORT);
      $files = array_combine($files, array_map("filemtime", $files));
      arsort($files);
      $latest_iso = key($files);

      // compute file size and date
      $fsize = filesize($latest_iso) >> 20;
      $ftime = date("d M Y", $files[$latest_iso]);

      echo "<p style=\"margin: 1.2em auto 0 auto; font-size: 1.2em; text-align: center; font-weight: bold;\"><a href=\"http://svarog386.viste.fr/{$latest_iso}\">Download the latest Svarog386 ISO</a></p>\n";
      echo '<p style="margin: 0 auto 1.4em auto; font-size: 1em; text-align: center; color: #333;">
      ';

      echo "({$fsize}M, last update: {$ftime}, <a href=\"{$latest_iso}.md5\" style=\"color: inherit;\">MD5</a>)";

      echo '
      </p>

      <p>You might want to know what it is exactly that you are about to download, before fetching the 450M+ ISO file. Feel free to browse the <a href="http://svarog386.viste.fr/repos/listing.txt">listing</a> of all the packages that come with Svarog386.</p>';

      }
      ?>

    </div></div>
  </body>
</html>

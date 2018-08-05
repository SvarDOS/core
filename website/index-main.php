    <p class="title">Svarog386 - a FreeDOS distribution for 386+ computers</p>
    <p class="copyr">brought to you by <a href="http://mateusz.viste.fr/" class="mateusz">Mateusz Viste</a></p>

    <div style="overflow: auto;">
    <img src="svarog386.png" alt="logo" style="height: 10em; float: right; margin: 0 0 0.5em 0.5em;">

    <p>Svarog386 is a <a href="?p=tech&amp;art=licensing">free</a>, <a href="?p=nls">multilingual</a> DOS distribution based on FreeDOS. It's released in the form of a single bootable CD image (ISO) that contains the FreeDOS kernel, a command interpreter and a variety of third-party packages.<br>
    Svarog386 is a "rolling release", meaning that it doesn't follow the concept of "versions". Svarog386 can be kept up-to-date either via online <a href="?p=tech&amp;art=onlinerepos">update repositories</a> or by using the latest ISO image as your local package repository.</p>

    <p>Svarog386 is <b>not</b> designed for strict 8086 compatibility. Many parts of it may require a 386-class CPU. If you look for a simple 8086 FreeDOS distribution, take a look at <a href="http://svarog86.sourceforge.net">Svarog86</a>.</p>

    <p>Need to get in touch? Wish to contribute some packages, translate Svarog386 to your language, or otherwise contribute? Feel free to drop a line or two to the usenet group <b><a href="?p=tech&amp;art=support">alt.os.free-dos</a></b>.
    </div>

<?php

  $downarr = parse_ini_file('downloads.ini', TRUE);

  echo "    <div style=\"width: 45%; float: left; margin: 1.2em 5% 0 0;\">\n";
  echo "      <p style=\"text-align: center; font-weight: bold;\"><a href=\"/{$downarr['nosrc']['url']}\">Download Svarog386</a></p>\n";
  echo "      <p style=\"margin: 0 auto 1.4em auto; font-size: 1em; text-align: center; color: #333;\">(" . $downarr['nosrc']['size'] >> 20 . "M, last update: " . date("d M Y", $downarr['nosrc']['date']) . ", <a href=\"/{$downarr['nosrc']['md5']}\" style=\"color: inherit;\">MD5</a>)</p>\n";
  echo "    </div>\n";

  echo "    <div style=\"width: 45%; float: right; margin: 1.2em 0 0 5%;\">\n";
  echo "      <p style=\"text-align: center; font-weight: bold;\"><a href=\"/{$downarr['full']['url']}\">Download Svarog386 (+sources)</a></p>\n";
  echo "      <p style=\"margin: 0 auto 1.4em auto; font-size: 1em; text-align: center; color: #333;\">(" . $downarr['full']['size'] >> 20 . "M, last update: " . date("d M Y", $downarr['full']['date']) . ", <a href=\"/{$downarr['full']['md5']}\" style=\"color: inherit;\">MD5</a>)</p>\n";
  echo "    </div>\n";

  echo "    <div style=\"width: 100%; margin: 1.2em 0 0 0;\">\n";
  echo "      <p style=\"text-align: center; font-weight: bold;\"><a href=\"/{$downarr['micro']['url']}\">Download Svarog386 micro (core OS only)</a></p>\n";
  echo "      <p style=\"margin: 0 auto 1.4em auto; font-size: 1em; text-align: center; color: #333;\">(" . $downarr['micro']['size'] >> 20 . "M, last update: " . date("d M Y", $downarr['micro']['date']) . ", <a href=\"/{$downarr['micro']['md5']}\" style=\"color: inherit;\">MD5</a>)</p>\n";
  echo "    </div>\n";

?>

    <p>You might want to know what it is exactly that you are about to download, before fetching the multi-megabytes ISO file. Feel free to browse the <a href="listing.txt">listing</a> of all the packages that come with Svarog386.</p>

    <p>If your computer is unable to boot from a CD, you can use Svarog386's <a href="boot.img">boot floppy image</a> to install Svarog386 (you still need to put Svarog386's CD in your drive).</p>

    <p class="wondering">Wondering how Svarog386 is built? You might want to take a look at the <a href="https://sourceforge.net/p/svarog386/code">project's SVN</a>, where all the build-related files are stored.</p>

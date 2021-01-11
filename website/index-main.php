    <h1>SvarDOS - an open-source DOS distribution</h1>
    <p class="copyr">brought to you by <a href="http://mateusz.viste.fr/" class="mateusz">Mateusz Viste</a></p>

    <p>SvarDOS is a <a href="?p=tech&amp;art=licensing">free</a>, <a href="?p=nls">multilingual</a> DOS distribution that uses the FreeDOS kernel. It's released in the form of a bootable CD image (ISO) that contains the "core" operating system, as well as a variety of third-party packages.<br>
    SvarDOS is a "rolling release", meaning that it doesn't follow the concept of "versions". SvarDOS can be kept up-to-date trough its online update tool.</p>

    <p>Need to get in touch? Wish to contribute some packages, translate SvarDOS to your language, or otherwise contribute? Feel free to drop a line or two to the project's <a href="https://lists.osdn.me/mailman/listinfo/svardos-users">mailing list</a>.</p>

<?php

  $downarr = parse_ini_file('downloads.ini', TRUE);

  echo "    <div style=\"width: 45%; float: left; margin: 1.2em 5% 0 0;\">\n";
  echo "      <p style=\"text-align: center; font-weight: bold;\"><a href=\"{$downarr['nosrc']['url']}\">Download SvarDOS</a></p>\n";
  echo "      <p style=\"margin: 0 auto 1.4em auto; font-size: 1em; text-align: center; color: #333;\">(" . ($downarr['nosrc']['size'] >> 20) . "M, last update: " . date("d M Y", $downarr['nosrc']['date']) . ", <a href=\"{$downarr['nosrc']['md5']}\" style=\"color: inherit;\">MD5</a>)</p>\n";
  echo "    </div>\n";

  echo "    <div style=\"width: 45%; float: right; margin: 1.2em 0 0 5%;\">\n";
  echo "      <p style=\"text-align: center; font-weight: bold;\"><a href=\"{$downarr['full']['url']}\">Download SvarDOS (+sources)</a></p>\n";
  echo "      <p style=\"margin: 0 auto 1.4em auto; font-size: 1em; text-align: center; color: #333;\">(" . ($downarr['full']['size'] >> 20) . "M, last update: " . date("d M Y", $downarr['full']['date']) . ", <a href=\"{$downarr['full']['md5']}\" style=\"color: inherit;\">MD5</a>)</p>\n";
  echo "    </div>\n";

  echo "    <div style=\"width: 100%; margin: 1.2em 0 0 0;\">\n";
  echo "      <p style=\"text-align: center; font-weight: bold;\"><a href=\"{$downarr['micro']['url']}\">Download SvarDOS micro (core OS only)</a></p>\n";
  echo "      <p style=\"margin: 0 auto 1.4em auto; font-size: 1em; text-align: center; color: #333;\">(" . ($downarr['micro']['size'] >> 20) . "M, last update: " . date("d M Y", $downarr['micro']['date']) . ", <a href=\"{$downarr['micro']['md5']}\" style=\"color: inherit;\">MD5</a>)</p>\n";
  echo "    </div>\n";

?>

    <p>You might want to know what it is exactly that you are about to download, before fetching the multi-megabytes ISO file. Browse the <a href="listing.txt">listing</a> of all the packages that come with SvarDOS.</p>

    <p>If your computer is unable to boot from a CD, you can use the SvarDOS <a href="boot.img">boot floppy image</a> to install SvarDOS (you still need to put the SvarDOS CD in your drive, though).</p>

    <p class="wondering">Wondering how SvarDOS is built? You might want to take a look at the <a href="https://osdn.net/projects/svardos/scm/svn/tree/head/">project's SVN</a>, where all the build-related files are stored.</p>

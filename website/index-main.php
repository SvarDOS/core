    <p class="title">Svarog386 - a FreeDOS distribution for 386+ computers</p>
    <p class="copyr">brought to you by <a href="http://mateusz.viste.fr/" class="mateusz">Mateusz Viste</a></p>
    <img src="svarog386.png" alt="logo" style="height: 10em; float: right; margin: 0 0 0.5em 0.5em;">

    <p>Svarog386 is a DOS distribution based on FreeDOS. It's released in the form of a single bootable CD image (ISO) that contains the FreeDOS kernel, a command interpreter and a variety of third-party packages.<br>
    Svarog386 is a "rolling release", meaning that it doesn't follow the concept of "versions". Svarog386 can be kept up-to-date either via online update repositories or by using the latest ISO image as your local package repository.</p>

    <p>Svarog386 is <b>not</b> designed for strict 8086 compatibility. Many parts of it might require a 386-class CPU. If you look for a simple 8086 FreeDOS distribution, take a look at <a href="http://svarog86.sourceforge.net">Svarog86</a>.</p>

    <p>Need to get in touch, or wish to contribute some packages? Feel free to drop a line to the usenet group <b>alt.os.free-dos</b>. Alternatively, you could also reach me directly via <a href="https://sourceforge.net/u/userid-1220451/">sourceforge</a>.

<?php
  // find the latest ISO file available
  $files = glob("*.iso", GLOB_NOSORT);
  $files = array_combine($files, array_map("filemtime", $files));
  arsort($files);
  $latest_iso = key($files);

  // compute file size and date
  $fsize = filesize($latest_iso) >> 20;
  $ftime = date("d M Y", $files[$latest_iso]);

  echo "    <p style=\"margin: 1.2em auto 0 auto; font-size: 1.2em; text-align: center; font-weight: bold;\"><a href=\"http://svarog386.viste.fr/{$latest_iso}\">Download the latest Svarog386 ISO</a></p>\n";
  echo '    <p style="margin: 0 auto 1.4em auto; font-size: 1em; text-align: center; color: #333;">';

  echo "({$fsize}M, last update: {$ftime}, <a href=\"{$latest_iso}.md5\" style=\"color: inherit;\">MD5</a>)";

  echo "</p>\n";

?>

    <p>You might want to know what it is exactly that you are about to download, before fetching the 450M+ ISO file. Feel free to browse the <a href="http://svarog386.viste.fr/repos/listing.txt">listing</a> of all the packages that come with Svarog386.</p>

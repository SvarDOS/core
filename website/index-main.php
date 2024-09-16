    <h1>SvarDOS - an open-source DOS distribution</h1>
    <p class="copyr">for PCs of the 1980-2000 era</p>

    <p>SvarDOS is an open-source project that is meant to integrate the best out of the currently available DOS tools, drivers and games. DOS development has been abandoned by commercial players a long time ago, mostly during early nineties. Nowadays it survives solely through the efforts of hobbyists and retro-enthusiasts, but this is a highly sparse and unorganized ecosystem. SvarDOS aims to collect available DOS software and make it easy to find and install applications using a network-enabled package manager (like apt-get, but for DOS and able to run on a 8086 PC).</p>

    <h2>Minimalist and 8086-compatible</h2>

    <p>Once installed, SvarDOS is a minimalistic DOS system that offers only a DOS kernel, a command interpreter and the most basic tools for system administration. It is then up to the user to install additional packages. Care is taken so SvarDOS remains 8086-compatible, at least in its most basic (core) configuration.</p>

    <h2>Open-source</h2>

    <p>SvarDOS is published under the terms of the MIT license. This applies only to SvarDOS-specific files, though - the auxilliary packages supplied with SvarDOS may be subject to different licenses (GPL, BSD, Public Domain, Freeware...).</p>

    <h2>Multilingual</h2>

    <p>The system can be set up in a wide selection of languages: English, German, French, Polish, Russian, Italian and more.</p>

    <h2>No "versions"</h2>

    <p>SvarDOS is a rolling release, meaning that it does not adhere to the concept of "versions". Once the system is installed, its packages can be kept up-to-date using the SvarDOS online update tools (pkg &amp; pkgnet).</p>

    <h2>Community and help</h2>

    <p>Need to get in touch? Wish to submit some packages, translate SvarDOS to your language, or otherwise contribute? Or maybe you'd like some information about SvarDOS? Come visit the <a href="?p=forum">SvarDOS community forum</a>. You may also wish to take a look at the project's <a href="https://github.com/SvarDOS/bugz/issues">bug tracker</a>.</p>

    <?php
      // the "default" build proposed on the main page is read from the
      // default_build.txt file, while "bleeding edge" is the latest entry
      // available in the download dir
      $lastver = trim(file_get_contents('default_build.txt'));
      $latestbuild = scandir('download/', SCANDIR_SORT_DESCENDING)[0];
    ?>

    <h2>Downloads</h2>

    <p>SvarDOS is available in a variety of installation images for different sizes of floppy disks. The CD image presented below contains only a virtual floppy to install the bare-bone SvarDOS system. If you'd like a CD image that contains lots of extra packages, you may want to look <a href="http://svardos.org/?p=repo">here</a>.</p>

    <div class="download">
    <div>
      <h3>Stable build (<?php echo $lastver; ?>)</h3>
      <ul>
      <?php
        $arr = array('cd' => 'CD-ROM ISO', 'floppy-1.44M' => '1.44M floppy disks', 'floppy-1.2M' => '1.2M floppy disks', 'floppy-720K' => '720K floppy disks', 'floppy-360K' => '360K floppy disks', 'usb' => 'bootable USB image');

        foreach ($arr as $l => $d) {
          echo "<li><a href=\"download/{$lastver}/svardos-{$lastver}-{$l}.zip\">{$d}</a></li>\n";
        }
      ?>
      </ul>
    </div>
    <?php
      if ($latestbuild !== $lastver) {
        echo "    <div>\n";
        echo "      <h3>Bleeding edge</h3>\n";
        echo "      <ul><li><a href=\"?p=files&amp;dir={$latestbuild}\">build {$latestbuild}</a></li></ul>\n";
        echo "      <p>⚠ This build has not been thoroughly tested! Let us know how it works for you.</p>\n";
        echo "    </div>\n";
      }
    ?>
    </div>

    <p>The links above point to the latest builds of installation images. Archival builds can be found in the <a href="?p=files">files section</a>.</p>

    <p class="wondering">Wondering how SvarDOS is built? Take a look at the <a href="http://svn.svardos.org/">project's SVN</a>, where all the build-related files and scripts are stored. To pull the sources using the standard subversion client use this:<br>svn co svn://svn.svardos.org/svardos svardos</p>

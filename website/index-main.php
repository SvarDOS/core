    <h1>SvarDOS - an open-source DOS distribution</h1>
    <p class="copyr">for PCs of the 1980-2000 era</p>

    <p>SvarDOS is an open-source project that is meant to integrate the best out of the currently available DOS tools, drivers and games. DOS development has been abandoned by commercial players a very long time ago, mostly during early nineties. Nowadays, it survives solely through the efforts of hobbyists and retro-enthusiasts, but this is a highly sparse and unorganized ecosystem. SvarDOS aims to collect available DOS software, package it and make it easy to find and install applications using a network-enabled package manager (like apt-get, but for DOS and able to run even on a 8086 PC).</p>

    <h2>Minimalist and 8086-compatible</h2>

    <p>Once installed, SvarDOS is a minimalistic DOS system that offers only the FreeDOS kernel and the most basic tools for system administration. It is up to the user to install additional packages. Care is taken so SvarDOS remains 8086-compatible, at least in its most basic (core) configuration.</p>

    <h2>Open-source</h2>

    <p>SvarDOS files are published under the terms of the MIT license. This applies only to SvarDOS-specific files, though - the packages supplied with SvarDOS may be subject to different licenses (GPL, BSD, Public Domain, Freeware...).</p>

    <h2>Multilingual</h2>

    <p>Many languages are supported: English, German, French, Polish, Russian, Italian and more.</p>

    <h2>No "versions"</h2>

    <p>SvarDOS is a "rolling" release, meaning that it doesn't follow the concept of "versions". Once the system is installed, its packages can be kept up-to-date using the SvarDOS online update tools (pkg &amp; pkgnet).</p>

    <h2>Community and help</h2>

    <p>Need to get in touch? Wish to submit some packages, translate SvarDOS to your language, or otherwise contribute? Or maybe you'd like some information about SvarDOS? Come visit the <a href="?p=forum">SvarDOS community forum</a>. You may also wish to take a look at the project's <a href="https://osdn.net/projects/svardos/ticket/">ticket list</a>.</p>

    <?php // the "default" build proposed on the main page is read from default_build.txt
    $lastver = trim(file_get_contents('default_build.txt'));

    echo '<h2>Downloads (build ' . $lastver . ')</h2>'
    ?>

    <ul>
      <li><a href="download/<?php echo $lastver; ?>/svardos-<?php echo $lastver; ?>-cd.zip">SvarDOS install CD (ISO)</a></li>
      <li><a href="download/<?php echo $lastver; ?>/svardos-<?php echo $lastver; ?>-floppy-2880k.zip">SvarDOS install on 2.88M floppy disks</a></li>
      <li><a href="download/<?php echo $lastver; ?>/svardos-<?php echo $lastver; ?>-floppy-1440k.zip">SvarDOS install on 1.44M floppy disks</a></li>
      <li><a href="download/<?php echo $lastver; ?>/svardos-<?php echo $lastver; ?>-floppy-1200k.zip">SvarDOS install on 1.2M floppy disks</a></li>
      <li><a href="download/<?php echo $lastver; ?>/svardos-<?php echo $lastver; ?>-floppy-720k.zip">SvarDOS install on 720K floppy disks</a></li>
      <li><a href="download/<?php echo $lastver; ?>/svardos-<?php echo $lastver; ?>-floppy-360k.zip">SvarDOS install on 360K floppy disks</a></li>
      <li><a href="download/<?php echo $lastver; ?>/svardos-<?php echo $lastver; ?>-usb.zip">SvarDOS install on a bootable USB image</a></li>
      <li><a href="download/<?php echo $lastver; ?>/svardos-<?php echo $lastver; ?>-dosemu.zip">SvarDOS image for DOSEMU</a><span class="helpmsg" title="a pre-installed image for the DOSEMU emulator, usually needs to be unzipped in ~/.dosemu/drive_c/">?</span></li>
    </ul>

    <p>The links above point to the latest stable build of installation images (that is, build <?php echo $lastver?>) and that's the build we recommend. Otherwise, archival and staging builds can be found in our <a href="?p=files">files section</a>, but that's only if you like living dangerously.</p>

    <p class="wondering">Wondering how SvarDOS is built? Take a look at the <a href="http://svn.svardos.org/">project's SVN</a>, where all the build-related files and scripts are stored. To pull the sources using the standard subversion client use this:<br>svn co svn://svn.svardos.org/svardos svardos</p>

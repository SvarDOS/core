    <h1>SvarDOS - an open-source DOS distribution</h1>
    <p class="copyr">for PCs of the 1980-2000 era</p>

    <!--<div style="margin: -1em auto 2em auto; width: 21em; border: 1px #777 solid; background-color: #fff; padding: 0.1em 0.5em; color: #a00; font-size: 1.1em; border-radius: 0.3em;">This project is very much "work-in-progress", not everything works yet! Wanna help? Get in touch through the project's mailing list!</div>-->

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

    <p>Need to get in touch? Wish to submit some packages, translate SvarDOS to your language, or otherwise contribute? Or maybe you'd like some information about SvarDOS? The project has a <a href="https://lists.osdn.me/mailman/listinfo/svardos-users">mailing list</a> just for that. You may also wish to take a look at the <a href="phpamb.php?fname=help/help-en.amb&amp;f=todo.ama">project's "todo list"</a>.</p>

    <?php
    echo '<h2>Downloads (build date: ' . gmdate('d M Y', filemtime('download/svardos-cd.zip')) . ')</h2>'
    ?>

    <ul>
      <li><a href="download/svardos-cd.zip">SvarDOS install CD (ISO)</a></li>
      <li><a href="download/svardos-floppy-2880k.zip">SvarDOS install on 2.88M floppy disks</a></li>
      <li><a href="download/svardos-floppy-1440k.zip">SvarDOS install on 1.44M floppy disks</a></li>
      <li><a href="download/svardos-floppy-1200k.zip">SvarDOS install on 1.2M floppy disks</a></li>
      <li><a href="download/svardos-floppy-720k.zip">SvarDOS install on 720K floppy disks</a></li>
      <li><a href="download/svardos-usb.zip">SvarDOS install on a bootable USB image</a></li>
      <li><a href="download/svardos-dosemu.zip">SvarDOS image for DOSEMU</a><span class="helpmsg" title="a pre-installed image for the DOSEMU emulator, usually needs to be unzipped in ~/.dosemu/drive_c/">?</span></li>
    </ul>

    <p class="wondering">Wondering how SvarDOS is built? Take a look at the <a href="http://svn.svardos.org/">project's SVN</a>, where all the build-related files are stored.</p>

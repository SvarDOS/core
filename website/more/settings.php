<?php

$PROJNAME = 'SvarDOS MORE';
$SHORT = 'displays output one screen at a time';

$LONGHTML = '
<p>SvarDOS MORE is a filter that displays output one screen at a time. It is part
of the SvarDOS project <http://svardos.org>, and as such it shares SvarDOS\'
principles of minimalism, compactness and bare-bones functionality. It is:</p>

<ul>
<li>multilingual (use the LANG variable, eg. "SET LANG=PL" switches to Polish)</li>
<li>TINY! (fits within a single disk sector)</li>
</ul>

<p>Usage:</p>

<pre>
  MORE < FILE.TXT
  TYPE FILE.TXT | MORE
</pre>

<p>SvarDOS MORE is hand crafted in x86 assembly and built using the excellent
A72 assembler by Mr R. Swan <https://github.com/swanlizard/a72>.</p>
';

$COPYRDATE = '2023';

?>

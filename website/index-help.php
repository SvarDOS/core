<h1>SvarDOS help documentation</h1>
<p class="copyr">no documentation is perfect, this one is no exception</p>

<p style="text-align: center;">Select the help language you wish to consult:<br><br>

<?php

  $list = scandir('./help/');

  foreach ($list as $l) {
    if ((!is_dir("help/{$l}")) || (!preg_match('/help-../', $l))) continue;
    $lang = strtoupper(substr($l, 5));
    echo '<a style="font-size: 1.2em;" href="phpamb.php?fname=help/' . $l . '">' . $lang . "</a><br>\n";
  }

?>

<br><br>

Wanna help to improve the SvarDOS help documentation? <a href="phpamb.php?fname=help/help-en&amp;f=contact.ama">Get in touch with us</a>!

</p>

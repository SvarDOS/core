<h1>SvarDOS repository</h1>
<p class="copyr">"the world of packages"</p>

<p>This page lists all packages that are available in the SvarDOS repository. These packages can be downloaded from within SvarDOS using the pkgnet tool, or you can download them from here.</p>

<?php

$db = json_decode(file_get_contents('../packages/_index.json'), true);

echo "<table>\n";

echo "<thead><tr><th>PACKAGE</th><th>VERSION</th><th>DESCRIPTION</th></tr></thead>\n";

foreach ($db as $pkg => $meta) {

  $desc = $meta['desc'];
  $pref = array_shift($meta['versions']); // get first version (that's the preferred one)
  $ver = $pref['ver'];
  $bsum = $pref['bsum'];

  echo "<tr><td><a href=\"repo/?a=pull&amp;p={$pkg}\">{$pkg}</a></td><td>{$ver}</td><td>{$desc}</td></tr>\n";
}
echo "</table>\n";

?>

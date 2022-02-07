<h1>SvarDOS repository</h1>
<p class="copyr">"the world of packages"</p>

<p>This page lists all packages that are available in the SvarDOS repository. These packages can be downloaded from within SvarDOS using the pkgnet tool, or you can download them from here.</p>

<?php

$handle = fopen('../packages/index.tsv', "rb");
if ($handle === FALSE) {
  echo "<p>ERROR: INDEX FILE NOT FOUND</p>\n";
  exit(0);
}

echo "<table>\n";

echo "<thead><tr><th>PACKAGE</th><th>VERSION</th><th>DESCRIPTION</th></tr></thead>\n";

while (($arr = fgetcsv($handle, 1024, "\t")) !== FALSE) {
  // format: pkgname | version | desc | bsdsum
  echo "<tr><td><a href=\"repo/?a=pull&amp;p={$arr[0]}\">{$arr[0]}</a></td><td>{$arr[1]}</td><td>{$arr[2]}</td></tr>\n";
}
echo "</table>\n";

fclose($handle);

?>

<h1>SvarDOS repository</h1>
<p class="copyr">"the world of packages"</p>

<p>This page lists all packages that are available in the SvarDOS repository. These packages can be downloaded from within SvarDOS using the pkgnet tool, or you can download them from here.</p>

<p>If you wish to receive notifications about new or updated packages, you may subscribe to the <a href="http://svn.svardos.org/rss.php?repname=SvarDOS&path=%2Fpackages%2F">SvarDOS Packages RSS feed</a>.</p>

<?php
  // load the category list
  $cats = json_decode(file_get_contents('../packages/_cats.json'), true);
  sort($cats);

  $catfilter = '';
  if (!empty($_GET['cat'])) $catfilter = strtolower($_GET['cat']);
?>

<form action="?" method="get">
<br>
<p>Category filter:&nbsp;
<input type="hidden" name="p" value="repo">
<select name="cat">
  <option value="">ALL</option>
<?php
  foreach ($cats as $c) {
    $sel = '';
    if ($c == $catfilter) $sel = ' selected';
    echo '  <option value="' . $c . "\"{$sel}>" . $c . "</option>\n";
  }
?>
</select>
<input type="submit" value="refresh">
</p>
</form>


<?php

$db = json_decode(file_get_contents('../packages/_index.json'), true);

echo "<table style=\"width: 100%;\">\n";

echo "<thead><tr><th>PACKAGE</th><th>VERSION</th><th>DESCRIPTION</th></tr></thead>\n";

foreach ($db as $pkg => $meta) {

  if ((!empty($catfilter)) && (array_search($catfilter, $meta['cats']) === false)) continue;

  $desc = $meta['desc'];
  $pref = array_shift($meta['versions']); // get first version (that's the preferred one)
  $ver = $pref['ver'];
  $bsum = $pref['bsum'];

  echo "<tr><td><a href=\"repo/?a=pull&amp;p={$pkg}\">{$pkg}</a></td><td>{$ver}</td><td>{$desc}</td></tr>\n";
}
echo "</table>\n";

$errs = trim(file_get_contents('../packages/_buildidx.log'));
if (!empty($errs)) echo '<p style="color: #f00; font-weigth: bold;">Note to SvarDOS packagers: inconsistencies have been detected in the repository, please <a href="?p=repo&amp;showlogs=1#logs">review them</a>.</p>';

if ($_GET['showlogs'] == 1) {
  echo "<p style=\"font-size: 0.8em;\"><a name=\"logs\">DEBUG REPO BUILD LOGS</a>:<br>\n";
  if (empty($errs)) {
    echo "no buildidx errors to display\n";
  } else {
    echo nl2br(htmlentities($errs));
  }
  echo "</p>\n";
}

?>

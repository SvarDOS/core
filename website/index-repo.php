<h1>SvarDOS repository</h1>
<p class="copyr">"the world of packages"</p>

<p>This page lists all packages that are available in the SvarDOS repository. These packages can be downloaded from within SvarDOS using the pkgnet tool, or you can download them from here.</p>

<p>You may also download a CD ISO image that contains the latest version of each package: <a href="repo/sv-repo.iso">SV-REPO.ISO</a> (<?php echo intval(filesize('repo/sv-repo.iso') / 1024 / 1024); ?>M, <a href="repo/sv-repo.iso.md5">md5</a>). This ISO file is bootable and based on the latest stable release, hence you may also use it to install SvarDOS on your system.</p>

<p>If you wish to receive notifications about new or updated packages, you may subscribe to the <a href="http://svn.svardos.org/rss.php?repname=SvarDOS%20Packages&amp;path=%2F&amp;isdir=1">SvarDOS Packages RSS feed</a>.</p>

<?php
  // look for BUILDIDX warnings
  $errs = trim(file_get_contents('../packages/_buildidx.log'));
  if (!empty($errs)) echo '<p style="color: #fff; font-weight: bold; border: 1px #555 solid; padding: 0.2em 0.5em; background: #f66; border-radius: 0.5em;">Note to SvarDOS packagers: inconsistencies have been detected in the repository, <a href="#logs">review them</a>.</p>';

  // load the category list
  $cats = json_decode(file_get_contents('../packages/_cats.json'), true);
  sort($cats);

  $catfilter = '';
  if (!empty($_GET['cat'])) $catfilter = strtolower($_GET['cat']);

  // hw filter?
  $hw = array();
  if (!empty($_GET['hwcpu'])) $hw[] = strtolower($_GET['hwcpu']);
  if (!empty($_GET['hwvid'])) $hw[] = strtolower($_GET['hwvid']);
  if ((!empty($_GET['fpu'])) && ($_GET['fpu'] == 'yes')) $hw[] = 'fpu';

  // view mode (0=simple 1=full)
  $view = 0;
  if (!empty($_GET['view'])) $view = intval($_GET['view']);
?>

<form action="?" method="get">
<br>
<p>Filters:<br>
<input type="hidden" name="p" value="repo">

Target hardware:

<select name="hwcpu">

<?php
  // build form with list of CPUs + preselect the current CPU (if any)
  $cpus = array('586', '486', '386', '286', '186', '8086');
  foreach ($cpus as $cpu) {
    $sel = '';
    if (array_search($cpu, $hw) !== false) $sel = ' selected';
    echo "<option{$sel}>{$cpu}</option>\n";
  }
?>
</select>

<select name="hwfpu">
<?php
if (empty($_GET['hwfpu'])) {
  echo "<option selected value=\"\">no FPU</option>\n";
  echo "<option>FPU</option>\n";
} else {
  echo "<option value=\"\">no FPU</option>\n";
  echo "<option selected>FPU</option>\n";
}
?>
</select>

<select name="hwvid">
<?php
  // build form with list of graphic cards + preselect the current card (if any)
  $vids = array('SVGA', 'VGA', 'MCGA', 'EGA', 'CGA', 'MDA');
  foreach ($vids as $v) {
    $sel = '';
    if (array_search(strtolower($v), $hw) !== false) $sel = ' selected';
    echo "<option{$sel}>{$v}</option>\n";
  }
?>
</select>

<br>

Category:
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

<input type="submit" value="apply">
</p>
</form>


<?php

// add all supported subsets of hardware (eg. VGA supports MDA, EGA and CGA modes)
foreach ($hw as $h) {

  // add all supported CPUs
  if ($h == '586') {
    $hw[] = '8086';
    $hw[] = '186';
    $hw[] = '286';
    $hw[] = '386';
    $hw[] = '486';
  }
  if ($h == '486') {
    $hw[] = '8086';
    $hw[] = '186';
    $hw[] = '286';
    $hw[] = '386';
  }
  if ($h == '386') {
    $hw[] = '8086';
    $hw[] = '186';
    $hw[] = '286';
  }
  if ($h == '286') {
    $hw[] = '8086';
    $hw[] = '186';
  }
  if ($h == '186') {
    $hw[] = '8086';
  }

  // add all supported graphic
  if ($h == 'svga') {
    $hw[] = 'vga';
    $hw[] = 'ega';
    $hw[] = 'cga';
    $hw[] = 'mda';
  }
  if ($h == 'vga') {
    $hw[] = 'mcga';
    $hw[] = 'ega';
    $hw[] = 'cga';
    $hw[] = 'mda';
  }
  if ($h == 'mcga') {
    $hw[] = 'ega';
    $hw[] = 'cga';
    $hw[] = 'mda';
  }
  if ($h == 'ega') {
    $hw[] = 'cga';
    $hw[] = 'mda';
  }
  if ($h == 'cga') {
    $hw[] = 'mda';
  }
}


$db = json_decode(file_get_contents('../packages/_index.json'), true);

echo "<table style=\"width: 100%;\">\n";

echo "<thead><tr><th>PACKAGE</th><th>VERSION</th><th>DESCRIPTION</th></tr></thead>\n";

foreach ($db as $pkg => $meta) {

  if ((!empty($catfilter)) && (array_search($catfilter, $meta['cats']) === false)) continue;

  $desc = htmlspecialchars($meta['desc']);
  check_next_ver:
  $pref = array_shift($meta['versions']); // get first version (that's the preferred one)
  if (empty($pref)) continue; // no more versions
  $hwhint = '';
  if (!empty($pref['hwreq'])) {
    $hwhint = ' title="' . htmlspecialchars(strtoupper(implode(', ', $pref['hwreq']))) . '"';

    /* if hw filter present, make sure it fullfills package's requirements */
    if (!empty($hw)) {
      foreach ($pref['hwreq'] as $req) {
        if (array_search($req, $hw, true) === false) goto check_next_ver;
      }
    }
  }
  $ver = $pref['ver'];
  $bsum = $pref['bsum'];

  if ($view === 0) {
    echo "<tr><td><a{$hwhint} href=\"repo/?a=pull&amp;p={$pkg}\">{$pkg}</a></td><td>{$ver}</td><td>{$desc}</td></tr>\n";
  } else {
    $link = '';
    if (!empty($meta['url'])) $link = '<span class="extrainfo"><br><a href="' . htmlspecialchars($meta['url']) . '">' . htmlspecialchars($meta['url']) . '</a></span>';
    echo "<tr><td><a{$hwhint} href=\"repo/?a=pull&amp;p={$pkg}\">{$pkg}</a><span class=\"extrainfo\"><br>";
    foreach ($meta['cats'] as $c) {
      echo '<a href="?p=repo&amp;cat=' . urlencode($c) . '&amp;view=1">' . htmlspecialchars($c) . '</a>';
    }
    echo "</span></td><td>{$ver}</td><td>{$desc}{$link}</td></tr>\n";
  }
}
echo "</table>\n";

if (!empty($errs)) {
  echo '<p style="font-size: 0.8em; border-top: 1px #000 solid;"><a name="logs">DEBUG REPO BUILD LOGS</a>:<br>' . "\n";
  echo nl2br(htmlentities($errs));
  echo "</p>\n";
}

?>

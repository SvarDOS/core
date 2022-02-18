<h1>SvarDOS files</h1>
<p class="copyr">latest build, archival versions, staging releases... it's all here!</p>

<?php

function nice_filesize($f) {
  $res = filesize($f);
  if ($res > 1024 * 1024) {
    $res = round($res / (1024 * 1024), 2) . ' MiB';
  } else {
    $res = round($res / 1024, 2) . ' KiB';
  }
  return($res);
}


chdir('download');
$dir = '.';

if (!empty($_GET['dir'])) {
  $dir = $_GET['dir'];
  if (!preg_match('/^[0-9]{8}$/', $dir)) {
    echo '<p style="font-size: 2em; text-align: center; font-weight: bold;">I AM WATCHING YOU</p>';
    exit(0);
  }
}

if ($dir == '.') {
  $flist = scandir($dir, SCANDIR_SORT_DESCENDING);
} else {
  $flist = scandir($dir);
}

echo "<div style=\"margin: 0 auto; width: -moz-fit-content; width: fit-content;\">\n";

if (strlen($dir) > 1) {
  echo "<h2>BUILD: {$dir}</h2>\n";
  echo '<a href="?p=files">[back to root]</a><br>' . "\n";
} else {
  echo "<h2>AVAILABLE BUILDS:</h2>\n";
}

echo "<br>\n";

echo "<table style=\"border: 1px #888 solid; min-width: 10em;\">\n";

foreach ($flist as $f) {
  if ($f[0] == '.') continue;
  if (preg_match('/\.php$/', $f)) continue;

  echo '<tr><td style="padding: 0 1em;">';

  if (is_dir($dir . '/' . $f)) {
    echo "<a href=\"?p=files&amp;dir={$f}\">{$f}</a>";
  } else {
    echo "<a href=\"download/{$dir}/{$f}\">{$f}</a>";
    echo "</td><td style=\"padding: 0 1em 0 2em; color: #222;\">" . nice_filesize($dir . '/' . $f);
  }

  echo "</td></tr>\n";

}

echo "</table>\n";

echo "</div>\n";

?>

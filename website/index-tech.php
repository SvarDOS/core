<?php
  $arts = array(
  'support'     => 'How do I get in touch with the Svarog386 community?',
  'pkgincl'     => 'Packages inclusion rules',
  'pkgformat'   => 'Package format',
  'licensing'   => 'Svarog386 licensing',
  'onlinerepos' => 'Svarog386 online repositories',
  'whatmeans'   => 'What does "Svarog" mean?'
  );

  natcasesort($arts);

  if (isset($_GET['art'])) {
    $art = $_GET['art'];
    if (!array_key_exists($art, $arts)) {
      echo "<p>invalid article!</p>";
      return;
    } else {
      echo "<p class=\"title\">" . htmlentities($arts[$art]) . "</p>\n";
      echo "<p class=\"copyr\">This is an article from the Svarog386 tech base</p>\n";
      echo '<p class="tech">';
      echo htmlentities(file_get_contents('tech/' . $art . '.txt'));
      echo "</p>\n";
      return;
    }
  }
?>

<p class="title">Svarog386 technical notes</p>
<p class="copyr">no documentation is perfect, this one's no exception</p>
<table style="margin: 0 auto;">

<?php
foreach ($arts as $file=>$title) {
  echo "<tr><td><a href=\"?p=tech&art={$file}\">" . htmlentities($title) . "</a></td></tr>";
}
?>

</table>

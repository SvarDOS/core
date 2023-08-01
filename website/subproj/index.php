<!DOCTYPE html><?php include 'settings.php' ?>
<html lang="en">
  <head>
    <title><?php echo $PROJNAME; ?></title>
    <meta name="keywords" content="svardos,command.com,shell">
    <meta name="author" content="Mateusz Viste">
    <meta name="robots" content="index, follow">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="style.css">
  </head>
  <body>

  <h1><?php echo $PROJNAME; ?></h1>
  <p class="copyr"><?php echo $SHORT ?></p>

  <?php echo $LONGHTML; ?>

  <h2>DOWNLOAD</h2>

  <table>
  <?php
    $flist = scandir("./", SCANDIR_SORT_DESCENDING);
    foreach ($flist as $f) {
      if (strpos($f, '.zip') == false) continue;
      $fsz = intval(filesize($f) / 1024);
      echo "<tr><td><a href=\"{$f}\">{$f}</a></td><td class=\"siz\"> {$fsz} KiB</td></tr>\n";
    }
  ?>
  </table>

  <h2>LICENSE (MIT)</h2>

  <p>Copyright &copy; <?php echo $COPYRDATE; ?> Mateusz Viste</p>

  <p>Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:</p>

  <ul>
  <li>- The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.</li>

  <li>- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.</li>
  </ul>

  </body>
</html>

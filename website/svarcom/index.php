<!DOCTYPE html>
<html lang="en">
  <head>
    <title>SvarCOM</title>
    <meta name="keywords" content="svardos,command.com,shell">
    <meta name="author" content="Mateusz Viste">
    <meta name="robots" content="index, follow">
    <meta charset="UTF-8">
    <link rel="stylesheet" href="style.css">
  </head>
  <body>

  <!-- ad button starts here -->
  <script src="http://mateusz.viste.fr/getmsg.php?align=right" type="text/javascript"></script>
  <!-- ad button ends here -->

  <h1>SvarCOM</h1>
  <p class="copyr">a nimble shell for SvarDOS</p>

  <p>SvarCOM is the SvarDOS command line interpreter, known usually under the name "COMMAND.COM". It is designed and maintained by <a href="http://mateusz.viste.fr" class="mateusz">Mateusz Viste</a>, and distributed under the terms of the MIT license.</p>

  <p>For the time being, it is a work-in-progress project that - although functional - is not entirely polished yet and might miss a few bits and pieces. SvarCOM version 2022.0 must be considered a "preview" version. See the documentation included in the zip file for details.</p>

  <p>SvarCOM is minimalist and I'd like to keep it that way. It aims to be functionaly equivalent to COMMAND.COM from MS-DOS 5.x/6.x. No LFN support.</p>

  <p>As of version 2022.0, SvarCOM's resident footprint is under 2 KiB.</p>

  <h2>DOWNLOAD</h2>

  <table>
  <?php
    $flist = scandir("./", SCANDIR_SORT_DESCENDING);
    foreach ($flist as $f) {
      if (strpos($f, '.zip') == false) continue;
      if (!stristr($f, 'svarcom')) continue;
      $fsz = intval(filesize($f) / 1024);
      echo "<tr><td><a href=\"{$f}\">{$f}</a></td><td class=\"siz\"> {$fsz} KiB</td></tr>\n";
    }
  ?>
  </table>

  <h2>LICENSE (MIT)</h2>

  <p>Copyright &copy; 2021-2022 Mateusz Viste</p>

  <p>Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:</p>

  <p>- The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.</p>

  <p>- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.</p>

  </body>
</html>

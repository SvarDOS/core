<?php
// php reader of AMB files -- turns an AMB book into a web page
//
// Copyright (C) 2020-2022 Mateusz Viste
// http://amb.osdn.io
//
// MIT license
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

$VERSION = "20220212";

function getamafile($ambfname, $amafname) {
  if (! is_file($ambfname)) {
    // if its a flat dir, just load the file
    if (is_dir($ambfname)) return(file_get_contents($ambfname . '/' . $amafname));
    return(FALSE);
  }
  $fd = fopen($ambfname, "rb");
  if ($fd === FALSE) return(FALSE);
  // read header (AMB1)
  if (fread($fd, 4) !== 'AMB1') return(FALSE);
  // read number of ama files inside
  $fcount = reset(unpack('v', fread($fd, 2)));
  if ($fcount === FALSE) return(FALSE);
  // read index until AMA file is found
  $offset = 0;
  $amalen = 0;
  $amafile = '';
  for ($i = 0; $i < $fcount; $i++) {
    $amafile = rtrim(fread($fd, 12), "\0");
    $offset = reset(unpack('V', fread($fd, 4)));
    $amalen = reset(unpack('v', fread($fd, 2)));
    $bsum = reset(unpack('v', fread($fd, 2)));
    if (strcasecmp($amafile, $amafname) == 0) break;
  }
  if ($i >= $fcount) return(FALSE); // not found
  // jump to offset and read file
  fseek($fd, $offset);
  $result = fread($fd, $amalen);
  fclose($fd);
  return($result);
}


// converts str into utf-8 using the unicodemap lookup table and returns the resulting (converted) str
function txttoutf8($str, $unicodemap) {
  $s = str_split($str, 1); // convert the string to a table for
  $res = '';
  foreach ($s as $c) $res .= $unicodemap[ord($c)]; // convert raw characters into HTML unicode codes
  return($res);
}


// MAIN STARTS HERE


if (empty($_GET['fname'])) {
  echo "usage: phpamb.php?fname=file.amb\n";
  exit(0);
}

$ambfname = $_GET['fname'];
$f = 'index.ama'; // default file
if (! empty($_GET['f'])) $f = $_GET['f'];

$title = trim(getamafile($ambfname, 'title'));
if ($title === FALSE) $title = $ambfname;

$ama = getamafile($ambfname, $f);
if ($ama === FALSE) $ama = 'ERROR: FILE NOT FOUND';

// prepare a 256-entries lookup array for unicode encoding
$unicodemap = array();
for ($i = 0; $i < 128; $i++) $unicodemap[$i] = $i; // low ascii is the same

$unicodemaptemp = unpack('v128', getamafile($ambfname, 'unicode.map'));
if ($unicodemaptemp === FALSE) {
  $unicodemap = FALSE;
} else {
  $unicodemap = array_merge($unicodemap, $unicodemaptemp);
  /* convert the unicode map so it contains actual html code instead of glyph values */
  for ($i = 0; $i < 256; $i++) {
    if ($unicodemap[$i] < 128) {
      $unicodemap[$i] = htmlspecialchars(chr($unicodemap[$i]), ENT_HTML5);
    } else {
      $unicodemap[$i] = '&#' . $unicodemap[$i] . ';';
    }
  }
  // perform UTF-8 conversion of the title
  $title = txttoutf8($title, $unicodemap);
}


echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head>\n";
echo '  <meta charset="UTF-8">' . "\n";
echo "  <title>{$title}</title>\n";
echo '  <link rel="stylesheet" href="phpamb.css">' . "\n";
echo '  <meta name="viewport" content="width=device-width, initial-scale=1">' . "\n";
echo "  <meta name=\"generator\" content=\"phpamb/{$VERSION}\">\n";
echo "</head>\n";
echo "<body>";

echo "<div><span><a href=\"?fname={$ambfname}\" class=\"liketext\">{$title}</a></span><span>[" . pathinfo($f, PATHINFO_FILENAME) . "]</span></div>\n";

/* detect links first, before any htmlization occurs */
$ama = preg_replace('!(https?|ftp)://([-A-Z0-9./_*?&;%=#~:]+)!i', 'LiNkStArTxXx$0LiNkEnDxXx', $ama);

if ($unicodemap !== FALSE) {
  $amacontent = str_split($ama, 1);
} else {
  $amacontent = mb_str_split($ama, 1, 'utf-8');
}
$escnow = 0;  // next char is an escape code
$readlink = 0; // a link target is being read
$opentag = ''; // do I have a currently open html tag?

$out = '';
foreach ($amacontent as $c) {
  // ignore CR
  if ($c == "\r") continue;
  // is link target being read?
  if ($readlink != 0) {
    if (($c == "\n") || ($c == ':')) {
      $out .= '">';
      $opentag = 'a';
      $readlink = 0;
      if ($c == ':') continue;
    } else {
      $out .= urlencode($c);
    }
    continue;
  }
  //
  if ($escnow != 0) {
    if ($c == '%') {
      $out .= '%';
    } else if ($c == 'l') {
      $out .= '<a href="' . $_SERVER['PHP_SELF'] . "?fname={$ambfname}&amp;f=";
      $readlink = 1;
    } else if ($c == 'h') {
      $out .= '<h1>';
      $opentag = 'h1';
    } else if ($c == '!') {
      $out .= '<span class="notice">';
      $opentag = 'span';
    } else if ($c == 'b') {
      $out .= '<span class="boring">';
      $opentag = 'span';
    }
    $escnow = 0;
    continue;
  }
  // close </a> if open and got LF or new tag
  if ((!empty($opentag)) && (($c == "\n") || ($c == "%"))) {
    $out .= "</{$opentag}>";
    $opentag = '';
  }
  //
  if ($c == '%') {
    $escnow = 1;
  } else {
    if ($unicodemap !== FALSE) {
      $out .= $unicodemap[ord($c)];  // convert characters into HTML unicode codes
    } else {
      $out .= htmlspecialchars($c);
    }
  }
}

// close open tags
if (!empty($opentag)) {
  $out .= "</{$opentag}>";
  $opentag = '';
}

/* postprocessing: find links detected earlier and change them to proper anchors */
$out = preg_replace('/LiNkStArTxXx.*LiNkEnDxXx/', '<a href="$0">$0</a>', $out);
$out = preg_replace('/(LiNkStArTxXx)|(LiNkEnDxXx)/', '', $out);

echo $out;

echo "</body>\n";
echo "</html>\n";

?>

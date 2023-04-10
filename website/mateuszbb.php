<?php
//
// mateuszbb - minimalistic bulletinboard-like forum
// Copyright (C) 2021-2023 Mateusz Viste
//

global $TRIP_SALT;
global $INITYEAR;
global $DATADIR;
global $NICE_URLS;
global $STR;
global $LANG;

include 'mateuszbb-config.php';


// *** TRANSLATION STRINGS *****************************************

$STR = array();
$STR['en']['opnewthread'] = 'open a new thread';
$STR['en']['newthread']   = 'New thread';
$STR['en']['latestentry'] = 'latest entry:';
$STR['en']['searchterm']  = 'searched term:';
$STR['en']['noresults']   = 'No results';
$STR['en']['reply']       = 'reply';
$STR['en']['jumptoend']   = 'jump to end';
$STR['en']['listthreads'] = 'list of threads';
$STR['en']['author']      = 'author:';
$STR['en']['address']     = 'address:';
$STR['en']['date']        = 'date:';
$STR['en']['nameornick']  = 'your name or nick';
$STR['en']['threadsubj']  = 'subject';
$STR['en']['yourmsg']     = 'Your message';
$STR['en']['cancel']      = 'cancel';
$STR['en']['send']        = 'send';
$STR['en']['archives']    = 'archives';
$STR['en']['backtocur']   = 'go back to current threads';
$STR['en']['search']      = 'search';
$STR['en']['password']    = 'password';
$STR['en']['optional']    = 'optional';
$STR['en']['passhelp']    = 'Providing a password here will generate a unique digital signature on your message.';
$STR['en']['captcha'][1]  = 'check the FIRST box';
$STR['en']['captcha'][2]  = 'check the MIDDLE box';
$STR['en']['captcha'][3]  = 'check the LAST box';
$STR['en']['captcha'][4]  = 'check the FIRST and LAST boxes';
$STR['en']['captcha'][5]  = 'check the TWO LAST boxes';

$STR['pl']['opnewthread'] = 'stwórz nowy wątek';
$STR['pl']['newthread']   = 'Nowy wątek';
$STR['pl']['latestentry'] = 'ostatni wpis:';
$STR['pl']['searchterm']  = 'szukane wyrażenie:';
$STR['pl']['noresults']   = 'Brak wyników';
$STR['pl']['reply']       = 'odpowiedz';
$STR['pl']['jumptoend']   = 'skocz do końca';
$STR['pl']['listthreads'] = 'lista wątków';
$STR['pl']['author']      = 'autor:';
$STR['pl']['address']     = 'adres:';
$STR['pl']['date']        = 'data:';
$STR['pl']['nameornick']  = 'imię, nazwisko lub pseudonim';
$STR['pl']['threadsubj']  = 'tytuł wątku';
$STR['pl']['yourmsg']     = 'Twoja wiadomość';
$STR['pl']['cancel']      = 'anuluj';
$STR['pl']['send']        = 'wyślij';
$STR['pl']['archives']    = 'archiwum';
$STR['pl']['backtocur']   = 'powrót do bieżących wątków';
$STR['pl']['search']      = 'szukaj';
$STR['pl']['password']    = 'hasło';
$STR['pl']['optional']    = 'opcjonale';
$STR['pl']['passhelp']    = 'Podanie hasła pozwoli wygenerować unikalny podpis elektroniczny przy twojej wiadomości.';
$STR['pl']['captcha'][1]  = 'zaznacz PIERWSZE pole';
$STR['pl']['captcha'][2]  = 'zaznacz ŚRODKOWE pole';
$STR['pl']['captcha'][3]  = 'zaznacz OSTATNIE pole';
$STR['pl']['captcha'][4]  = 'zaznacz PIERWSZE i OSTATNIE pole';
$STR['pl']['captcha'][5]  = 'zaznacz DWA OSTATNIE pola';

// *****************************************************************


function data_dluga($timestamp) {
  date_default_timezone_set('UTC');
  return(date('d.m.Y, H:i:s', $timestamp) . ' UTC');
}


// returns an array with the list of languages requested by the browser, in
// the order of preference
function getpreflang() {
  $res = array();
  if (! isset($_SERVER['HTTP_ACCEPT_LANGUAGE'])) return($res);
  $langlist = explode(',', $_SERVER['HTTP_ACCEPT_LANGUAGE']);
  foreach ($langlist as $lang) {
    $res[] = strtolower(substr($lang, 0, 2));
  }
  return(array_unique($res));
}


function mateuszbb_rss() {
  global $DATADIR;
  global $RSS_TITLE;

  $db = new SQLite3($DATADIR . 'mateuszbb.sqlite3', SQLITE3_OPEN_READONLY);
  if (! $db) {
    echo "SQL ERROR: ACCESS DENIED\n";
    return false;
  }
  $sqlres = $db->query('SELECT thread, msgid, author FROM rss ORDER BY msgid DESC, thread DESC LIMIT 100;');
  if (! $sqlres) {
    echo "SQL ERROR: QUERY FAILED\n";
    return false;
  }

  header('content-type: application/rss+xml');

  echo '<?xml version="1.0" encoding="utf-8" ?>' . "\n";
  echo '<rss version="2.0">' . "\n";
  echo "<channel>\n";
  echo "<title>" . htmlspecialchars($RSS_TITLE, ENT_XML1) . "</title>\n";
  echo "<link>" . selfurl(). "</link>\n";

  while ($row = $sqlres->fetchArray()) {
    $rawtitle = file_get_contents($DATADIR . 'threads/' . $row['thread'] . '/title.txt');
    if (empty($rawtitle)) continue;
    $title = htmlspecialchars($rawtitle, ENT_XML1, 'UTF-8');
    $author = htmlspecialchars($row['author'], ENT_XML1, 'UTF-8');
    if ($NICE_URLS) {
      $link = selfurl();
      if (substr($link, -1) !== '/') $link .= '/';
      $link .= "{$row['thread']}";
    } else {
      $link = selfurl('thread=' . $row['thread']);
    }
    $link .= '#' . $row['msgid'];
    echo "<item>\n";
    echo "<title>{$author} @ '{$title}'</title>\n";
    echo "<link>" . htmlspecialchars($link, ENT_XML1) . "</link>\n";
    echo "<description>{$author} @ '{$title}'</description>\n";
    echo "<pubDate>" . date('r', $row['msgid']) . "</pubDate>\n";
    echo "<guid>" . htmlspecialchars($link, ENT_XML1) . "</guid>\n";
    echo "</item>\n";
  }
  $db->close();

  echo "</channel>\n";
  echo "</rss>\n";
  return true;
}


function formularz($thread = '') {
  global $STR;
  global $LANG;

  if (empty($thread)) {
    echo '<form class="minibb" method="POST" action="' . selfurl() . '#title" id="formularz">' . "\n";
    echo '<input type="hidden" name="action" value="createthread">' . "\n";
  } else {
    echo '<form class="minibb" method="POST" action="' . selfurl() . '" id="formularz">' . "\n";
    echo '<input type="hidden" name="action" value="newpost">' . "\n";
    echo '<input type="hidden" name="thread" value="' . $thread . '">' . "\n";
  }

  echo '<div class="minibb-formfields">' . "\n";
  echo '<div class="minibb-formlabelgroup"><p>' . $STR[$LANG]['nameornick'] . '</p><input type="text" name="login" pattern=".*[^\s].*" minlength="1" maxlength="40" title="' . $STR[$LANG]['nameornick'] . '" required></div><div class="minibb-formlabelgroup"><p>' . $STR[$LANG]['password'] . ' (<span title="' . $STR[$LANG]['passhelp'] . '" style="text-decoration-line: underline; text-decoration-style: dotted;">' . $STR[$LANG]['optional'] . '</span>)</p><input type="password" name="pass" maxlength="40"></div>' . "\n";
  if (empty($thread)) {
    echo '<div class="minibb-formlabelgroup" style="width: 100%;">' . "\n";
    echo "<p>" . $STR[$LANG]['threadsubj'] . "</p>\n";
    echo '<input type="text" name="title" title="' . $STR[$LANG]['threadsubj'] . '" maxlength="64" pattern=".*[^\s].*" required>' . "\n";
    echo "</div>\n";
  }
  echo '<textarea name="msg" placeholder="' . $STR[$LANG]['yourmsg'] . '">' . "\n";
  echo '</textarea><br>' . "\n";
  echo '</div>' . "\n";
  // --- CAPTCHA ---
  $capid = rand(1, 5);
  echo '<div class="minibb-formcaptcha">' . $STR[$LANG]['captcha'][$capid] . ': <input type="checkbox" name=c1> <input type="checkbox" name=c2> <input type="checkbox" name=c3>' . "\n";
  echo '<input type="hidden" name="capid" value="' . $capid . '">';
  // ---------------
  echo '<div class="minibb-formbtns">';
  echo '<a href="' . selfurl() . '">' . $STR[$LANG]['cancel'] . '</a> <input type="submit" value="' . $STR[$LANG]['send'] . '">' . "\n";
  echo '</div>';
  echo '</form>';
}

function wyswietl_watek_w_liscie($threadid, $tytul, $lastauthor, $lastupdate) {
  global $NICE_URLS;
  global $STR;
  global $LANG;

  echo '<a href="';
  if (!$NICE_URLS) {
    echo selfurl("thread=$threadid");
  } else {
    echo $threadid;
  }
  echo '" class="minibb-threaditem">' . "\n";
  echo '<h2>' . htmlspecialchars($tytul) . "</h2>\n";
  echo '<p>' . $STR[$LANG]['latestentry'] . ' ' . htmlspecialchars($lastauthor) . ', ' . htmlspecialchars(data_dluga($lastupdate)) . "</p>\n";
  echo "</a>\n";
}

function sprawdz_captcha($CAPARR) {
  //echo "<!-- capid={$CAPARR['capid']} c1={$CAPARR['c1']} c2={$CAPARR['c2']} c3={$CAPARR['c3']}-->\n";
  switch ($CAPARR['capid']) {
    case 1:
      if (($CAPARR['c1']) && (!$CAPARR['c2']) && (!$CAPARR['c3'])) return(true);
      break;
    case 2:
      if ((!$CAPARR['c1']) && ($CAPARR['c2']) && (!$CAPARR['c3'])) return(true);
      break;
    case 3:
      if ((!$CAPARR['c1']) && (!$CAPARR['c2']) && ($CAPARR['c3'])) return(true);
      break;
    case 4:
      if (($CAPARR['c1']) && (!$CAPARR['c2']) && ($CAPARR['c3'])) return(true);
      break;
    case 5:
      if ((!$CAPARR['c1']) && ($CAPARR['c2']) && ($CAPARR['c3'])) return(true);
      break;
  }
  return(false);
}


// zwraca akcję na podstawie globalnych POST lub GET
function getvar_action() {
  if (!empty($_POST['action'])) return $_POST['action'];
  if (!empty($_GET['action'])) return $_GET['action'];
  return('');
}

function getvar_thread() {
  if (!empty($_POST['thread'])) return intval($_POST['thread']);
  if (!empty($_GET['thread'])) return intval($_GET['thread']);
  return(-1);
}

function getvar_archiveyear() {
  if (!empty($_POST['arch'])) return intval($_POST['arch']);
  if (!empty($_GET['arch'])) return intval($_GET['arch']);
  return(-1);
}


// funkcja która zapisuje nowe wiadomości
function mateuszbb_preprocess() {
global $TRIP_SALT;
global $DATADIR;
global $NICE_URLS;
global $ERRSTR; // zmienna zawierająca komunikat błędu (jeśli jakiś wystąpił)
global $STR;
global $LANG;

$action = getvar_action();
$thread = getvar_thread();
$archiveyear = getvar_archiveyear();

// negotiate language
$LANG = 'en'; // preselect english as default language
foreach (getpreflang() as $l) {
  if (!empty($STR[$l])) {
    $LANG = $l;
    break;
  }
}

// write access: check how many messages the user posted during last 24h
if (($action === 'createthread') || ($action === 'newpost')) {
   $db = new SQLite3($DATADIR . 'mateuszbb.sqlite3');
   if ($db) {
     $db->exec('DELETE FROM ip_msg_counters24h WHERE msgid < strftime(\'%s\', \'now\') - 24*3600;');
     $count24h = intval($db->querySingle("SELECT count(*) FROM ip_msg_counters24h WHERE ipaddr = '{$_SERVER['REMOTE_ADDR']}'"));
     $db->close();
     if ($count24h >= 10) {
       $ERRSTR = "BŁĄD: Z TWOJEGO ADRESU NAPISANO JUŻ {$count24h} WIADOMOŚCI W PRZECIĄGU OSTATNICH 24H. SPRÓBUJ PONOWNIE ZA JAKIŚ CZAS.";
       $action = '';
     }
   }
}

// new thread creation (+switch to read thread)
if ($action === 'createthread') {
  // captcha check
  if (!sprawdz_captcha($_POST)) {
    echo "<p>BŁĄD: NIEPRAWIDŁOWE CAPTCHA</p>\n";
    goto DONE;
  }
  //
  $thread = time();
  if (empty($_POST['login']) || (empty($_POST['msg'])) || (empty($_POST['title']))) {
    echo '<p>BŁĄD: pusty nick, wiadomość lub tytuł</p>' . "\n";
    goto DONE;
  }
  if (!mkdir($DATADIR . 'threads/' . $thread, 0755, true)) {
    echo '<p>BŁĄD: nie zdołano utworzyć wątku nr ' . $thread . "</p>\n";
    goto DONE;
  }
  // zapisz tytuł
  file_put_contents($DATADIR . 'threads/' . $thread . '/title.txt', trim($_POST['title']));
  // ustaw co trzeba żeby zapisać wiadomość
  $action = 'newpost';
}

// nowy post do istniejącego wątku
if (($action === 'newpost') && ($thread >= 0) && (!empty($_POST['msg'])) && (!empty($_POST['login']))) {
  $postid = time();
  if (!sprawdz_captcha($_POST)) {
    $ERRSTR = "BŁĄD: NIEPRAWIDŁOWE CAPTCHA";
    goto DONE;
  }
  // nadpisz lastauthor i lastupdate
  $lastupdate = array('lastupdate' => $postid, 'lastauthor' => trim($_POST['login']));
  file_put_contents($DATADIR . 'threads/' . $thread . '/lastupdate', serialize($lastupdate));
  // oblicz tripkod, jeśli hasło zostało ustawione
  $tripsig = '';
  if (!empty(trim($_POST['pass']))) {
    $tripsig = hash('whirlpool', trim($_POST['login']) . '#' . trim($_POST['pass']) . $TRIP_SALT);
  }
  // zapisz wiadomość
  $msg = array('author' => trim($_POST['login']), 'ip' => $_SERVER['REMOTE_ADDR'], 'trip' => $tripsig, 'msg' => trim($_POST['msg']));
  file_put_contents($DATADIR . 'threads/' . $thread . '/' . $postid, serialize($msg));
  // zaktualizuj metadane dot. ostatniego wpisu, ostatniego autora i ilości wpisów dla tego IP w ciągu ostatniej godziny
  $db = new SQLite3($DATADIR . 'mateuszbb.sqlite3');
  if ($db) {
    $db->exec('CREATE TABLE IF NOT EXISTS newest (thread INTEGER PRIMARY KEY, lastupdate INTEGER NOT NULL, lastauthor TEXT NOT NULL);');
    $db->exec('CREATE INDEX IF NOT EXISTS lastupdated ON newest (lastupdate);');
    $db->exec('CREATE TABLE IF NOT EXISTS ip_msg_counters24h (threadid INTEGER NOT NULL, msgid INTEGER NOT NULL, ipaddr TEXT NOT NULL);');
    $db->exec('CREATE TABLE IF NOT EXISTS rss (thread INTEGER NOT NULL, msgid INTEGER NOT NULL, author TEXT NOT NULL);');
    $db->exec('CREATE INDEX IF NOT EXISTS rss_msgid ON rss (msgid);');
    $login_escaped = $db->escapeString(trim($_POST['login']));
    $db->exec("INSERT OR REPLACE INTO newest (thread, lastupdate, lastauthor) VALUES ({$thread}, {$postid}, '{$login_escaped}');");
    $db->exec("INSERT INTO rss (thread, msgid, author) VALUES ({$thread}, {$postid}, '{$login_escaped}');");
    $db->exec("INSERT INTO ip_msg_counters24h (threadid, msgid, ipaddr) VALUES ({$thread}, {$postid}, '{$_SERVER['REMOTE_ADDR']}');");
    $db->close();
  } else {
    echo "SQL ERROR WHILE WRITING STATS\n";
  }
  // przekieruj
  if ($NICE_URLS) {
    $newurl = "{$thread}#{$postid}";
  } else {
    $newurl = selfurl("thread={$thread}") . "#{$postid}";
  }
  header("Location: {$newurl}");
  echo "<html><head></head><body><a href=\"{$newurl}\">KLIKNIJ TUTAJ</a></body></html>\n";
  exit();
}

DONE:

}


function mateuszbb_tytulwatku($id) {
  global $DATADIR;
  return file_get_contents($DATADIR . 'threads/' . $id . '/title.txt');
}


// wyświetlanie UI itd
function mateuszbb_start() {
global $TRIP_SALT;
global $ERRSTR;
global $DATADIR;
global $INITYEAR;
global $LANG;
global $STR;
global $NICE_URLS;
global $SEARCH_API_URL;

// read global variables
$action = getvar_action();
$thread = getvar_thread();
$archiveyear = getvar_archiveyear();

// wyświetl błąd, jeśli jakiś wystąpił w mateuszbb_preprocess()
if (!empty($ERRSTR)) {
  echo "<p class=\"minibb-errstr\">{$ERRSTR}</p>\n";
  $action = '';
  echo '<p><a href="./">Wróć do głównej strony</a></p>' . "\n";
  goto DONE;
}

// szukanie
if (isset($_POST['szukaj']) && (!empty(trim($_POST['szukaj'])))) {
  $q = trim($_POST['szukaj']);
  $query = $SEARCH_API_URL . urlencode($q);
  echo '<p>' . $STR[$LANG]['searchterm'] . ' ' . htmlentities($q) . '</p>';
  $results = file_get_contents($query);
  $resarr = json_decode($results, true)['items'];

  $licznik = 0;
  foreach ($resarr as $r) {
    if (mb_substr($r['link'], -1) === '/') continue;
    echo '<a href=' . $r['link'] . ' class="minibb-searchresult">';
    echo "<div><h1>{$r['title']}</h1><p>{$r['htmlSnippet']}</p></div></a>\n";
    $licznik++;
  }

  if ($licznik == 0) echo "<p>" . $STR[$LANG]['noresults'] . "</p>\n";

  goto DONE;
}

// new thread form
if ($action === 'newthread') {
  echo '<h2 class="minibb-threadtitle">' . $STR[$LANG]['newthread'] . '</h2>' . "\n";
  formularz();
  goto DONE;
}

// zobacz listę wątków
if ((empty($action)) && ($thread < 0) && ($archiveyear <= 0)) {
  echo '<div class="minibb-toolbar" style="justify-content: space-between;">';
  echo '<form action="' . selfurl() . '" method="POST"><input type="text" name="szukaj" placeholder="' . $STR[$LANG]['search'] . '"></form>';
  echo '<a href="' . selfurl('action=newthread') . '#formularz">' . $STR[$LANG]['opnewthread'] . '</a>';
  echo "</div>\n";
  $db = new SQLite3($DATADIR . 'mateuszbb.sqlite3', SQLITE3_OPEN_READONLY);
  if ($db) {
    $sqlres = $db->query('SELECT thread, lastupdate, lastauthor FROM newest ORDER BY lastupdate DESC LIMIT 30;');
    if (!$sqlres) {
      echo "SQL ERROR\n";
    } else {
      while ($row = $sqlres->fetchArray()) {
        $title = mateuszbb_tytulwatku($row['thread']);
        if (empty($title)) {
          echo "<!-- BŁĄD: nie zdołano załadować wątku nr {$row['thread']} -->\n";
          continue;
        }
        wyswietl_watek_w_liscie($row['thread'], $title, $row['lastauthor'], $row['lastupdate']);
      }
    }
    $db->close();
  } else {
    echo "<p>BŁĄD DOSTĘPU DO BAZY DANYCH</p>";
  }

  echo '<div style="display: flex; justify-content: space-between; font-size: 0.9em; opacity: 0.8; margin: 0.6em 0.5em 0 0.5em;">' . "\n";
  echo '<div>' . $STR[$LANG]['archives'] . ':';
  for ($y = $INITYEAR; $y <= intval(gmdate('Y')); $y++) {
    if ($NICE_URLS) {
      echo " <a href=\"{$y}\">{$y}</a>";
    } else {
      echo ' <a href="' . selfurl("arch={$y}") . '">' . $y . '</a>';
    }
  }
  echo "</div>\n";
  echo '<a href="rss.php"><img style="height: 1em;" src="mateuszbb_rss.svg"></a>' . "\n";
  echo "</div>\n";

  // display the main page footer if any is defined
  if (file_exists('mateuszforum-mainfooter.htm')) {
    readfile('mateuszbb-mainfooter.htm');
  }

  goto DONE;
}

// wyświetl archiwum
if ($archiveyear > 0) {
  echo '<div class="minibb-toolbar" id="title"><a href="' . selfurl() . '">' . $STR[$LANG]['backtocur'] . '</a></div>' . "\n";
  echo "<h1>" . $STR[$LANG]['archives'] . " {$archiveyear}</h1>\n";
  $threads = scandir($DATADIR . 'threads/', SCANDIR_SORT_ASCENDING);
  foreach ($threads as $t) {
    if (!preg_match('/^[0-9][0-9]*$/', $t)) continue; // skip anything that is not a thread id
    if (intval(gmdate('Y', $t)) != $archiveyear) continue; // skip threads out of the targeted year
    $title = file_get_contents($DATADIR . 'threads/' . $t . '/title.txt');
    $link = $t;
    if (! $NICE_URLS) $link = selfurl("thread={$t}");
    echo '<span style="font-family: monospace;">[' . gmdate("Y-m-d", $t) . "]</span> <a href=\"{$link}\">{$title}</a><br>\n";
  }
  goto DONE;
}

// zobacz wątek
if ((empty($action)) && ($thread >= 0)) {
  // załaduj listę postów (i zapamiętaj ostatnią pozycję)
  $listapostow = scandir($DATADIR . 'threads/' . $thread . '/');
  // usuń pozycje które nie są żadnym msgid (np. title.txt) i zapamiętaj ostatni msgid
  $posty = array();
  foreach ($listapostow as $p) {
    if (!preg_match('/^[0-9][0-9]*$/', $p)) continue; // skip anything that is not a messageid
    $posty[] = $p;
    $ostatnipost = $p;
  }
  // toolbar (ostatni wątek / odpowiedz / powrót do forum)
  echo '<div class="minibb-toolbar" id="title">';
  echo '<a href="#' . $ostatnipost . '">' . $STR[$LANG]['jumptoend'] . '</a> <a href="#formularz">' . $STR[$LANG]['reply'] . '</a> <a href="' . selfurl() . '">' . $STR[$LANG]['listthreads'] . '</a></div>' . "\n";
  // wyświetl tytuł wątku
  echo '<h2 class="minibb-threadtitle">' . htmlspecialchars(file_get_contents($DATADIR . 'threads/' . $thread . '/title.txt')) . "</h2>\n";
  // wyświetl listę wątków
  foreach ($posty as $p) {
    $msg = unserialize(file_get_contents($DATADIR . 'threads/' . $thread . '/' . $p));
    echo '<div class="minibb-post" id="' . $p . '">' . "\n";
    echo '<div class="minibb-postheader"><a href="#' . $p . '" style="text-decoration: inherit; color: inherit;"><div class="minibb-postauthor">' . "\n";
    echo $STR[$LANG]['author'] . ' ' . htmlspecialchars($msg['author']) . "<br>\n";
    echo $STR[$LANG]['address'] . ' ' . htmlspecialchars($msg['ip']) . "<br>\n";
    echo $STR[$LANG]['date'] . ' ' . htmlspecialchars(data_dluga($p)) . "</div></a>\n";
    if (!empty($msg['trip'])) {
      echo '<div class="minibb-trip">';
      echo chunk_split($msg['trip'], 16, "\n");
      echo "</div>\n";
    }
    echo "</div>\n";

    // symbole html
    $bodyprocessed = htmlspecialchars($msg['msg']);

    // dodaj podgląd pod linki do obrazków, ale tylko jeśli link jest sam w linijce
    $bodyprocessed = preg_replace('~^(http[s]?://[^<>[:space:]]+[[:alnum:]/]\.(jpg|png))($|[\r\n])~m', "$1\n<img src=\"$1\">\n", $bodyprocessed);

    // olinkuj linki
    $bodyprocessed = preg_replace("~([^\"]|^)(http[s]?://[^<>[:space:]]+[[:alnum:]/])~", "$1<a href=\"$2\">$2</a>", $bodyprocessed);

    // oflaguj cytaty (linijki zaczynające się od ">")
    $bodyprocessed = preg_replace('/^(&gt; .*)[\r]?\n/m', '<blockquote>$1</blockquote>', $bodyprocessed);

    echo '<div class="minibb-postbody">' . $bodyprocessed . '</div>' . "\n";
    echo "</div>\n";
  }
  // formularz odpowiedzi i do domu
  formularz($thread);
  goto DONE;
}

DONE:
}
?>

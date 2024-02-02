<?php
//
// mateuszbb - minimalist bulletin board forum. MIT license.
//
// VERSION 20240123
//
// Copyright (C) 2021-2024 Mateusz Viste
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

global $TRIP_SALT;
global $INITYEAR;
global $DATADIR;
global $NICE_URLS;
global $STR;
global $LANG;
global $TZ;
global $DATE_FORMAT;
global $MAINPAGE_MAXTHREADS;
global $MAINPAGE_MAXINACT;
global $LOCK_DELAY;
global $EDIT_ALLOWED_MINUTES;
global $MAXDAILYPOSTS;

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
$STR['en']['locked']      = "Thread locked due to inactivity since over {$LOCK_DELAY} days.";
$STR['en']['captcha'][1]  = 'check the FIRST box';
$STR['en']['captcha'][2]  = 'check the MIDDLE box';
$STR['en']['captcha'][3]  = 'check the LAST box';
$STR['en']['captcha'][4]  = 'check the FIRST and LAST boxes';
$STR['en']['captcha'][5]  = 'check the TWO LAST boxes';

// DE translations by Robert Riebisch
$STR['de']['opnewthread'] = 'Neues Thema eröffnen';
$STR['de']['newthread']   = 'Neues Thema';
$STR['de']['latestentry'] = 'Neuester Eintrag:';
$STR['de']['searchterm']  = 'Gesuchter Begriff:';
$STR['de']['noresults']   = 'Keine Ergebnisse';
$STR['de']['reply']       = 'Antworten';
$STR['de']['jumptoend']   = 'Zum Ende springen';
$STR['de']['listthreads'] = 'Liste der Themen';
$STR['de']['author']      = 'Autor:';
$STR['de']['address']     = 'Adresse:';
$STR['de']['date']        = 'Zeitpunkt:';
$STR['de']['nameornick']  = 'Dein Name oder Spitzname';
$STR['de']['threadsubj']  = 'Thema';
$STR['de']['yourmsg']     = 'Deine Nachricht';
$STR['de']['cancel']      = 'Abbrechen';
$STR['de']['send']        = 'Senden';
$STR['de']['archives']    = 'Archiv';
$STR['de']['backtocur']   = 'Zurück zu den aktuellen Themen';
$STR['de']['search']      = 'Suchbegriff';
$STR['de']['password']    = 'Kennwort';
$STR['de']['optional']    = 'optional';
$STR['de']['passhelp']    = 'Wenn du hier ein Kennwort eingibst, wird deine Nachricht mit einer eindeutigen digitalen Signatur versehen.';
$STR['de']['locked']      = "Thema wegen Inaktivität seit über {$LOCK_DELAY} Tagen gesperrt.";
$STR['de']['captcha'][1]  = 'Kreuze das ERSTE Kästchen an';
$STR['de']['captcha'][2]  = 'Kreuze das MITTLERE Kästchen an';
$STR['de']['captcha'][3]  = 'Kreuze das LETZTE Kästchen an';
$STR['de']['captcha'][4]  = 'Kreuze das ERSTE und LETZTE Kästchen an';
$STR['de']['captcha'][5]  = 'Kreuze die beiden LETZTEN Kästchen an';

// PL translations by Mateusz Viste
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
$STR['pl']['optional']    = 'opcjonalne';
$STR['pl']['passhelp']    = 'Podanie hasła pozwoli wygenerować unikalny podpis elektroniczny przy twojej wiadomości.';
$STR['pl']['locked']      = "Wątek zamknięty z powodu braku aktywności od ponad {$LOCK_DELAY} dni.";
$STR['pl']['captcha'][1]  = 'zaznacz PIERWSZE pole';
$STR['pl']['captcha'][2]  = 'zaznacz ŚRODKOWE pole';
$STR['pl']['captcha'][3]  = 'zaznacz OSTATNIE pole';
$STR['pl']['captcha'][4]  = 'zaznacz PIERWSZE i OSTATNIE pole';
$STR['pl']['captcha'][5]  = 'zaznacz DWA OSTATNIE pola';

// pt-BR translations courtesty of Luzemário Dantas
$STR['pt']['opnewthread'] = 'abrir novo tópico';
$STR['pt']['newthread']   = 'Novo tópico';
$STR['pt']['latestentry'] = 'entrada mais recente:';
$STR['pt']['searchterm']  = 'termo pesquisado:';
$STR['pt']['noresults']   = 'Sem resultados';
$STR['pt']['reply']       = 'responder';
$STR['pt']['jumptoend']   = 'ir para o final';
$STR['pt']['listthreads'] = 'lista de tópicos';
$STR['pt']['author']      = 'autor:';
$STR['pt']['address']     = 'endereço:';
$STR['pt']['date']        = 'data:';
$STR['pt']['nameornick']  = 'seu nome ou apelido';
$STR['pt']['threadsubj']  = 'assunto';
$STR['pt']['yourmsg']     = 'Sua mensagem';
$STR['pt']['cancel']      = 'cancelar';
$STR['pt']['send']        = 'enviar';
$STR['pt']['archives']    = 'arquivos';
$STR['pt']['backtocur']   = 'voltar ao tópico atuai';
$STR['pt']['search']      = 'pesquisar';
$STR['pt']['password']    = 'senha';
$STR['pt']['optional']    = 'opcional';
$STR['pt']['passhelp']    = 'Fornecer uma senha aqui vai gerar uma assinatura digital única na sua mensagem.';
$STR['pt']['locked']      = "Este tópico está bloqueado porque está inativo há mais de {$LOCK_DELAY} dias."; // translated by google translate, wording might be poor
$STR['pt']['captcha'][1]  = 'marque a PRIMEIRA caixa';
$STR['pt']['captcha'][2]  = 'marque a caixa do MEIO';
$STR['pt']['captcha'][3]  = 'marque a ÚLTIMA caixa';
$STR['pt']['captcha'][4]  = 'marque a PRIMEIRA e ÚLTIMA caixas';
$STR['pt']['captcha'][5]  = 'marque as DUAS ÚLTIMAS caixas';

// *****************************************************************


function data_dluga($timestamp) {
  global $DATE_FORMAT;
  return(date($DATE_FORMAT, $timestamp));
}


function selfurl($params = '') {
  global $SELFURL;
  $r = $SELFURL;
  if (!empty($params)) {
    if (strrchr($SELFURL, '?')) {
      $r .= '&';
    } else {
      $r .= '?';
    }
    $r .= $params;
  }
  return($r);
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
  global $NICE_URLS;

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
  echo "<description>" . htmlspecialchars($RSS_TITLE, ENT_XML1) . "</description>\n";

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


function formularz($thread = 0, $postid = 0, $msg = '') {
  global $STR;
  global $LANG;
  global $NICE_URLS;

  if ($thread == 0) {
    echo '<form class="minibb" method="POST" action="' . selfurl() . '#title" id="formularz">' . "\n";
    echo '<input type="hidden" name="action" value="createthread">' . "\n";
  } else {
    echo '<form class="minibb" method="POST" action="' . selfurl() . '" id="formularz">' . "\n";
    echo '<input type="hidden" name="thread" value="' . $thread . '">' . "\n";
    if ($postid > 0) {
      echo '<input type="hidden" name="action" value="editpost">' . "\n";
      echo '<input type="hidden" name="postid" value="' . $postid . '">' . "\n";
    } else {
      echo '<input type="hidden" name="action" value="newpost">' . "\n";
    }
  }

  echo '<div class="minibb-formfields">' . "\n";
  echo '<div class="minibb-formlabelgroup"><p>' . $STR[$LANG]['nameornick'] . '</p><input type="text" name="username" pattern=".*[^\s].*" minlength="1" maxlength="40" autofill="username" title="' . $STR[$LANG]['nameornick'];
  if (!empty($msg)) echo '" value="' . htmlspecialchars($msg['author']) . '"';
  echo '" required></div><div class="minibb-formlabelgroup"><p>' . $STR[$LANG]['password'] . ' (<span title="' . $STR[$LANG]['passhelp'] . '" style="text-decoration-line: underline; text-decoration-style: dotted;">' . $STR[$LANG]['optional'] . '</span>)</p><input type="password" name="password" maxlength="40" autofill="current-password"></div>' . "\n";
  if ($thread == 0) {
    echo '<div class="minibb-formlabelgroup" style="width: 100%;">' . "\n";
    echo "<p>" . $STR[$LANG]['threadsubj'] . "</p>\n";
    echo '<input type="text" name="title" title="' . $STR[$LANG]['threadsubj'] . '" maxlength="64" pattern=".*[^\s].*" required>' . "\n";
    echo "</div>\n";
  }
  echo '<textarea name="msg" placeholder="' . $STR[$LANG]['yourmsg'] . '">' . "\n";
  if (!empty($msg)) echo htmlspecialchars($msg['msg']);
  echo "</textarea><br>\n";
  echo "</div>\n";
  // --- CAPTCHA ---
  $capid = rand(1, 5);
  echo '<div class="minibb-formcaptcha">' . $STR[$LANG]['captcha'][$capid] . ': <span class="minibb-cboxgroup"><input type="checkbox" name=c1><input type="checkbox" name=c2><input type="checkbox" name=c3></span>' . "\n";
  echo '<input type="hidden" name="capid" value="' . $capid . '">';
  // ---------------
  echo '<div class="minibb-formbtns">' . "\n";
  $link = selfurl();
  if ($postid > 0) {
    $link = selfurl("thread=" . $thread);
    if ($NICE_URLS) $link = $thread;
    $link .= '#' . $postid;
  }
  echo '<a href="' . $link . '">' . $STR[$LANG]['cancel'] . '</a> <input type="submit" value="' . $STR[$LANG]['send'] . '">' . "\n";
  echo "</div>\n";
  echo "</div>\n";
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
global $MAXDAILYPOSTS;
global $EDIT_ALLOWED_MINUTES;

$action = getvar_action();
$thread = getvar_thread();
$archiveyear = getvar_archiveyear();

// negotiate language, unless forced by configuration
if (empty($LANG)) {
  $LANG = 'en'; // preselect english as default language
  foreach (getpreflang() as $l) {
    if (!empty($STR[$l])) {
      $LANG = $l;
      break;
    }
  }
} else { // if language forced by configuration then make sure it is supported
  if (empty($STR[$LANG])) $LANG = 'en'; // fall back to 'en' on error
}

// write access: check how many messages the user posted during last 24h
if (($action === 'createthread') || ($action === 'newpost')) {
  $db = new SQLite3($DATADIR . 'mateuszbb.sqlite3');
  if ($db) {
    $db->exec('DELETE FROM ip_msg_counters24h WHERE msgid < strftime(\'%s\', \'now\') - 24*3600;');
    $count24h = intval($db->querySingle("SELECT count(*) FROM ip_msg_counters24h WHERE ipaddr = '{$_SERVER['REMOTE_ADDR']}'"));
    $db->close();
    if ($count24h >= $MAXDAILYPOSTS) {
      $ERRSTR = "BŁĄD: Z TWOJEGO ADRESU NAPISANO JUŻ {$count24h} WIADOMOŚCI W PRZECIĄGU OSTATNICH 24H. SPRÓBUJ PONOWNIE ZA JAKIŚ CZAS.";
      $action = '';
    }
  }
}

// edit post becomes newpost, it was different just to avoid 24h counters
if ($action === 'editpost') $action = 'newpost';

// new thread creation (+switch to read thread)
if ($action === 'createthread') {
  // captcha check
  if (!sprawdz_captcha($_POST)) {
    echo "<p>BŁĄD: NIEPRAWIDŁOWE CAPTCHA</p>\n";
    goto DONE;
  }
  //
  $thread = time();
  if (empty($_POST['username']) || (empty($_POST['msg'])) || (empty($_POST['title']))) {
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
if (($action === 'newpost') && ($thread >= 0) && (!empty($_POST['msg'])) && (!empty($_POST['username']))) {
  // is it really about a NEW post or about EDITING an existing one?
  if (empty($_POST['postid'])) {
    $postid = time();
  } else { // editing an existing post
    $msg = loadmsg($_POST['thread'], $_POST['postid']);
    if (!is_art_edition_allowed($_POST['postid'], $msg)) {
      $action = '';
      $ERRSTR = "NOT ALLOWED";
      goto DONE;
    }
    $postid = $_POST['postid'];
  }

  if (!sprawdz_captcha($_POST)) {
    $ERRSTR = "BŁĄD: NIEPRAWIDŁOWE CAPTCHA";
    goto DONE;
  }
  // nadpisz lastauthor i lastupdate
  $lastupdate = array('lastupdate' => $postid, 'lastauthor' => trim($_POST['username']));
  file_put_contents($DATADIR . 'threads/' . $thread . '/lastupdate', serialize($lastupdate));
  // oblicz tripkod, jeśli hasło zostało ustawione
  $tripsig = '';
  if (!empty(trim($_POST['password']))) {
    $tripsig = hash('whirlpool', trim($_POST['username']) . '#' . trim($_POST['password']) . $TRIP_SALT);
  }
  // wygeneruj klucz do edycji postu i prześlij go przeglądarce przez ciasteczko (chyba że przeglądarka już ma klucz)
  if (!empty($EDIT_ALLOWED_MINUTES)) {
    if (!empty($_COOKIE['mateuszbbkey'])) {
      $artkey = $_COOKIE['mateuszbbkey'];
    } else {
      $artkey = bin2hex(random_bytes(128));
      setcookie('mateuszbbkey', $artkey, array('secure' => false, 'httponly' => true, 'samesite' => 'Lax'));
    }
  }
  // zapisz wiadomość
  $msg = array('author' => trim($_POST['username']), 'ip' => $_SERVER['REMOTE_ADDR'], 'trip' => $tripsig, 'msg' => trim($_POST['msg']), 'key' => password_hash($artkey, PASSWORD_DEFAULT));
  file_put_contents($DATADIR . 'threads/' . $thread . '/' . $postid, serialize($msg));
  // zaktualizuj metadane dot. ostatniego wpisu, ostatniego autora i ilości wpisów dla tego IP w ciągu ostatniej godziny, ale tylko dla nowych wpisów (nie dla edycji)
  if (empty($_POST['postid'])) {
    $db = new SQLite3($DATADIR . 'mateuszbb.sqlite3');
    if ($db) {
      $db->exec('CREATE TABLE IF NOT EXISTS newest (thread INTEGER PRIMARY KEY, lastupdate INTEGER NOT NULL, lastauthor TEXT NOT NULL);');
      $db->exec('CREATE INDEX IF NOT EXISTS lastupdated ON newest (lastupdate);');
      $db->exec('CREATE TABLE IF NOT EXISTS ip_msg_counters24h (threadid INTEGER NOT NULL, msgid INTEGER NOT NULL, ipaddr TEXT NOT NULL);');
      $db->exec('CREATE TABLE IF NOT EXISTS rss (thread INTEGER NOT NULL, msgid INTEGER NOT NULL, author TEXT NOT NULL);');
      $db->exec('CREATE INDEX IF NOT EXISTS rss_msgid ON rss (msgid);');
      $login_escaped = $db->escapeString(trim($_POST['username']));
      $db->exec("INSERT OR REPLACE INTO newest (thread, lastupdate, lastauthor) VALUES ({$thread}, {$postid}, '{$login_escaped}');");
      $db->exec("INSERT INTO rss (thread, msgid, author) VALUES ({$thread}, {$postid}, '{$login_escaped}');");
      $db->exec("INSERT INTO ip_msg_counters24h (threadid, msgid, ipaddr) VALUES ({$thread}, {$postid}, '{$_SERVER['REMOTE_ADDR']}');");
      $db->close();
    } else {
      echo "SQL ERROR WHILE WRITING STATS\n";
    }
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


// returns an array of last n threads with most recent activity that had activity
// in last maxinact days. returns false on error or empty set.
// the returned result, when not false, is an array of arrays, where each
// leaf array represents one thread
function mateuszbb_getactivethreads($n, $maxinact = -1) {
  global $DATADIR;
  $result = array();

  $db = new SQLite3($DATADIR . 'mateuszbb.sqlite3', SQLITE3_OPEN_READONLY);
  if (! $db) return(false);

  $minupdatedate = 0;
  if ($maxinact >= 0) $minupdatedate = time() - (intval($maxinact) * 86400);

  $sqlquery = 'SELECT thread, lastupdate, lastauthor FROM newest WHERE lastupdate > ' . $minupdatedate . ' ORDER BY lastupdate DESC LIMIT ' . intval($n) . ';';

  $sqlres = $db->query($sqlquery);
  if (! $sqlres) {
    $db->close();
    return(false);
  }

  // kopiuj wpisy do nowej tablicy
  while ($row = $sqlres->fetchArray()) {
    $result[] = $row;
  }

  $db->close();
  return($result);
}


// returns true if post can be edited by current user
function is_art_edition_allowed($timestamp, $msg) {
  global $EDIT_ALLOWED_MINUTES;
  if ($EDIT_ALLOWED_MINUTES >= 0) {
    if (((time() - $timestamp) / 60) >= $EDIT_ALLOWED_MINUTES) return(false); // only posts from last x minutes can be edited
  }
  if (empty($_COOKIE['mateuszbbkey'])) return(false);
  if (empty($msg['key'])) return(false);
  return(password_verify($_COOKIE['mateuszbbkey'], $msg['key']));
}


function loadmsg($threadid, $postid) {
  global $DATADIR;
  $fname = $DATADIR . 'threads/' . $threadid . '/' . $postid;
  if (!file_exists($fname)) return(false);
  return(unserialize(file_get_contents($fname)));
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
global $TZ;
global $LOCK_DELAY;
global $MAINPAGE_MAXTHREADS;
global $MAINPAGE_MAXINACT;

// read global variables
$action = getvar_action();
$thread = getvar_thread();
$archiveyear = getvar_archiveyear();

// ustaw strefę czasową, jeśli jakaś jest skonfigurowana
if (!empty($TZ)) date_default_timezone_set($TZ);

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
    if (strlen($r['link']) <= strlen(selfurl())) continue;
    echo '<a href=' . $r['link'] . ' class="minibb-searchresult">';
    echo "<div><h1>{$r['title']}</h1><p>{$r['htmlSnippet']}</p></div></a>\n";
    $licznik++;
  }

  if ($licznik == 0) echo "<p>" . $STR[$LANG]['noresults'] . "</p>\n";

  goto DONE;
}

// edit post
if ($action === 'editpostform') {
  $msg = loadmsg($_POST['thread'], $_POST['post']);
  if (is_art_edition_allowed($_POST['post'], $msg)) {
    formularz(intval($_POST['thread']), intval($_POST['post']), $msg);
  } else {
    echo "<p>Link expired</p>\n";
  }
  GOTO DONE;
}

// new thread form
if ($action === 'newthread') {
  echo '<h2 class="minibb-threadtitle">' . $STR[$LANG]['newthread'] . '</h2>' . "\n";
  formularz();
  goto DONE;
}

// zobacz listę wątków (main page)
if ((empty($action)) && ($thread < 0) && ($archiveyear <= 0)) {
  // display the main page header if any is defined
  if (file_exists($DATADIR . 'mateuszbb-main-head.html')) {
    readfile($DATADIR . 'mateuszbb-main-head.html');
  }
  // list wątków
  echo '<div class="minibb-toolbar" style="justify-content: space-between;">';
  echo '<form action="' . selfurl() . '" method="POST"><input type="text" name="szukaj" placeholder="' . $STR[$LANG]['search'] . '"></form>';
  echo '<a href="' . selfurl('action=newthread') . '#formularz">' . $STR[$LANG]['opnewthread'] . '</a>';
  echo "</div>\n";

  $lista_watkow = mateuszbb_getactivethreads($MAINPAGE_MAXTHREADS, $MAINPAGE_MAXINACT);
  if ($lista_watkow === false) {
    echo "<p>NO ENTRIES FOUND</p>";
  } else {
    foreach ($lista_watkow as $row) {
      $title = mateuszbb_tytulwatku($row['thread']);
      if (empty($title)) {
        echo "<!-- BŁĄD: nie zdołano załadować wątku nr {$row['thread']} -->\n";
        continue;
      }
      wyswietl_watek_w_liscie($row['thread'], $title, $row['lastauthor'], $row['lastupdate']);
    }
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
  echo '<a href="rss.php"><img style="height: 1em;" src="mateuszbb_rss.svg" alt="RSS"></a>' . "\n";
  echo "</div>\n";

  // display the main page footer if any is defined
  if (file_exists($DATADIR . 'mateuszbb-main-foot.html')) {
    readfile($DATADIR . 'mateuszbb-main-foot.html');
  }

  goto DONE;
}

// wyświetl archiwum
if ($archiveyear > 0) {
  echo '<div class="minibb-toolbar" id="title"><a href="' . selfurl() . '">' . $STR[$LANG]['backtocur'] . '</a></div>' . "\n";
  echo '<h2 class="minibb-threadtitle">' . $STR[$LANG]['archives'] . " {$archiveyear}</h2>\n";
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
  // is this thread locked?
  $islocked = false;
  if (($LOCK_DELAY >= 0) && ((time() - intval($ostatnipost)) / 86400 >= $LOCK_DELAY)) $islocked = true;
  // toolbar (ostatni wątek / odpowiedz / powrót do forum)
  echo '<div class="minibb-toolbar" id="title">';
  echo '<a href="#' . $ostatnipost . '">' . $STR[$LANG]['jumptoend'] . '</a>';
  if (! $islocked) echo ' <a href="#formularz">' . $STR[$LANG]['reply'] . '</a>';
  echo ' <a href="' . selfurl() . '">' . $STR[$LANG]['listthreads'] . '</a></div>' . "\n";
  // wyświetl tytuł wątku
  echo '<h2 class="minibb-threadtitle">' . htmlspecialchars(file_get_contents($DATADIR . 'threads/' . $thread . '/title.txt')) . "</h2>\n";
  // "thread is locked"
  if ($islocked) echo '<p class="minibb-islockedmsg">' . $STR[$LANG]['locked'] . "</p>\n";
  // wyświetl listę postów
  foreach ($posty as $p) {
    $msg = loadmsg($thread, $p);
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

    // ludzie czasem dodają znaczniki [img] do obrazków, usuń je (ale tylko jeśli są na początku linii)
    $bodyprocessed = preg_replace('~^(\[img\])(.*)(\[/img\])~m', '$2', $bodyprocessed);

    // dodaj podgląd pod linki do obrazków, ale tylko jeśli link jest sam w linijce
    $bodyprocessed = preg_replace('~^(http[s]?://[^<>[:space:]]+[[:alnum:]/]\.(jpg|png))($|[\r\n]{1,2})~m', "$1\n<img src=\"$1\">\n", $bodyprocessed);

    // olinkuj linki
    $bodyprocessed = preg_replace("~([^\"]|^)(http[s]?://[^<>[:space:]]+[[:alnum:]/=])~", "$1<a href=\"$2\">$2</a>", $bodyprocessed);

    // oflaguj cytaty (linijki zaczynające się od ">")
    $bodyprocessed = preg_replace('/^(&gt;.*)[\r]?\n/m', '<blockquote>$1</blockquote>', $bodyprocessed);

    echo '<div class="minibb-postbody">';
    // czy mogę edytować?
    if (is_art_edition_allowed($p, $msg)) {
      echo '<form class="editbtn" method="POST" action="' . selfurl() . '"><input type="hidden" name="action" value="editpostform"><input type="hidden" name="post" value="' . $p . '"><input type="hidden" name="thread" value="' . $thread . '"><input type="submit" value="EDIT"></form>';
    }
    echo $bodyprocessed . "</div>\n";
    echo "</div>\n";
  }
  // formularz odpowiedzi albo komunikat o zamknięciu
  if ($islocked) {
    echo '<p class="minibb-islockedmsg">' . $STR[$LANG]['locked'] . "</p>\n";
  } else {
    formularz($thread);
  }
  goto DONE;
}

DONE:
}
?>

<?php

// ustaw na TRUE jeśli instalacja wspiera "ładne" adresy. Wymaga reguł MOD_REWRITE w .htaccess:
// RewriteEngine On
// RewriteRule "^([0-9][0-9][0-9][0-9])/$" "?action=archiwum&y=$1" [PT]
// RewriteRule "^([0-9][0-9][0-9][0-9][0-9]+)" "?thread=$1" [PT]
$NICE_URLS = FALSE;

// Data directory, ie the place where mateuszforum will store its sqlite db and
// forum threads. This directory must be writeable by the process running
// mateuszforum.
// This directory path can be empty, but if not then it MUST end with a path
// delimiter (typically, a slash)
$DATADIR = '/srv/svardos-forumdata/';

// year of first forum activity, used to generate a list of links to archives
$INITYEAR = 2023;

// parametr solny dla funkcji crypt() - istotne tylko przy generowaniu tripkodów
$TRIP_SALT = trim(file_get_contents($DATADIR . 'tripsalt.txt'));

$SEARCH_API_URL = 'https://www.googleapis.com/customsearch/v1/siterestrict?key=AIzaSyCxLEZe7_LdeOBtPzs4LEbwXmr1bGERfDE&cx=8928515a857418bb5&q=';

$RSS_TITLE = 'SvarDOS community forum';

$SELFURL = 'http://svardos.org/?p=forum';

?>

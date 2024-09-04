<?php

// Language of the UI interface. Leave empty for auto-detection.
// Supported languages: en pl pt
$LANG = '';

// Timezone to use for displaying timestamps. Server's system time zone is used
// if this field is left empty.
//
// Examples: UTC ; Europe/Paris ; America/Los_Angeles ; Australia/Perth
//
// See the manual of PHP's date_default_timezone_set() for more information.
$TZ = 'UTC';

// sets the format to display timestamps. See PHP's documentation for date()
// for more information. Example: 'd.m.Y, H:i:s'
$DATE_FORMAT = 'd.m.Y, H:i \U\T\C';

// Set this to TRUE for "nice" URLS, so instead of such URL:
//
//   http://myforum.example/?thread=12345678
//
// mateuszbb would use this kind of URLs:
//   http://myforum.example/12345678
//
// This requires the MOD_REWRITE extension to be enabled in your web server's
// configuration, and such rules to be placed in .htaccess:
//
// RewriteEngine On
// RewriteRule "^([0-9][0-9][0-9][0-9])/$" "?arch=$1" [PT]
// RewriteRule "^([0-9][0-9][0-9][0-9][0-9]+)" "?thread=$1" [PT]
$NICE_URLS = FALSE;

// Data directory, ie the place where mateuszbb will store its sqlite cache and
// forum threads. This directory must be writeable by the process running
// mateuszbb.
// This directory path can be empty, but if it is not then it MUST end with a
// directory delimiter (typically a slash)
$DATADIR = '/srv/svardos-forumdata/';

// the maximum number of threads to be displayed on the main page.
$MAINPAGE_MAXTHREADS = 30;

// the maximum number of inactivity days for threads displayed on the main page.
$MAINPAGE_MAXINACT = 600;

// year of first forum activity, used to generate a list of links to archives
$INITYEAR = 2023;

// Threads become "locked" after this many days without a post. A locked thread
// is still visible, but does not allow new messages to be posted in it.
// Set this to -1 for threads never to be locked.
$LOCK_DELAY = 365;

// Once a message is posted, it may be edited by its author during this many
// minutes or until the browser's session is closed, whichever comes first.
// For no time limit other than browser's session lifetime, set this to -1.
// This feature relies on cookie authentication, hence it will work only if the
// browser accepts cookies.
$EDIT_ALLOWED_MINUTES = 240;

// parametr solny dla funkcji crypt() - istotne tylko przy generowaniu tripkodów
$TRIP_SALT = trim(file_get_contents($DATADIR . 'tripsalt.txt'));

// the maximum number of posts an IP address is allowed to submit within 24h
$MAXDAILYPOSTS = 20;

$SEARCH_API_URL = 'https://www.googleapis.com/customsearch/v1/siterestrict?key=AIzaSyCxLEZe7_LdeOBtPzs4LEbwXmr1bGERfDE&cx=8928515a857418bb5&q=';

$RSS_TITLE = 'SvarDOS community forum';

$SELFURL = 'http://svardos.org/?p=forum';

?>

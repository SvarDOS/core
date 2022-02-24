<?php

// translations of repo messages into several languages
// all messages are stored as UTF-8 strings here, and converted to the target
// codepage by index.php using mappings defined in CP_UTF[] and CP_ENC[]

$CP_UTF = array();
$CP_ENC = array();
$MSG = array();


// *** CODEPAGE MAPPINGS ******************************************************

// EN (nothing to do, it's ASCII)
$CP_UTF['en'] = array();
$CP_ENC['en'] = array();

// DE (cp437, cp850, cp858 have all the same layout for these)
$CP_UTF['de'] = array('ä',   'ö',   'ü',   'Ä',   'Ö',   'Ü',   'ß');
$CP_ENC['de'] = array("\x84","\x94","\x81","\x8e","\x99","\x9a","\xe1");

// PL (mazovia)
$CP_UTF['pl'] = array('Ą',   'Ć',   'Ę',   'Ł',   'Ń',   'Ó',   'Ś',   'Ż',   'Ź',   'ą',   'ć',   'ę',   'ł',   'ń',   'ó',   'ś',   'ż',   'ź');
$CP_ENC['pl'] = array("\x8f","\x95","\x90","\x9c","\xa5","\xa3","\x98","\xa1","\xa0","\x86","\x8d","\x91","\x92","\xa4","\xa2","\x9e","\xa7","\xa6");


// *** MESSAGES ***************************************************************

$MSG['NO_MATCHING_PKG']['en'] = 'No matching package found';
$MSG['NO_MATCHING_PKG']['de'] = 'Kein passendes Paket gefunden';
$MSG['NO_MATCHING_PKG']['pl'] = 'Nie znaleziono żadnego pasującego pakietu';

$MSG['PKG_NOT_FOUND']['en'] = 'ERROR: package not found on server';
$MSG['PKG_NOT_FOUND']['de'] = 'FEHLER: Paket nicht auf dem Server gefunden';
$MSG['PKG_NOT_FOUND']['pl'] = 'BŁĄD: Nie znaleziono pakietu na serwerze';

$MSG['PKG_NO_SRC']['en'] = 'ERROR: no sources available for this package';
$MSG['PKG_NO_SRC']['de'] = 'FEHLER: keine Quelltexte verfügbar für dieses Paket';
$MSG['PKG_NO_SRC']['pl'] = 'BŁĄD: brak źródeł dla tego pakietu';

$MSG['VER']['en'] = 'ver:';
$MSG['VER']['de'] = 'Ver:';
$MSG['VER']['pl'] = 'wer:';

$MSG['SIZE']['en'] = 'size:';
$MSG['SIZE']['de'] = 'Größe:';
$MSG['SIZE']['pl'] = 'rozmiar:';

$MSG['ALT_VERS']['en'] = 'alt versions:';
$MSG['ALT_VERS']['de'] = 'alt. Versionen:';
$MSG['ALT_VERS']['pl'] = 'alt. wersje:';

$MSG['PACKAGE']['en'] = 'PACKAGE';
$MSG['PACKAGE']['de'] = 'PAKET';
$MSG['PACKAGE']['pl'] = 'PAKIET';

$MSG['INSTALLED']['en'] = 'INSTALLED (LOCAL)';
$MSG['INSTALLED']['de'] = 'INSTALLIERT (LOKAL)';
$MSG['INSTALLED']['pl'] = 'ZAINSTALOWANY (LOKALNY)';

$MSG['AVAILABLE']['en'] = 'AVAILABLE (REMOTE)';
$MSG['AVAILABLE']['de'] = 'VERFÜGBAR (ONLINE)';
$MSG['AVAILABLE']['pl'] = 'DOSTĘPNY (ZDALNY)';

$MSG['NO_UPDATES']['en'] = 'no available updates';
$MSG['NO_UPDATES']['de'] = 'keine Aktualisierungen verfügbar';
$MSG['NO_UPDATES']['pl'] = 'brak dostępnych aktualizacji';

$MSG['FOUND_DIFFER']['en'] = 'found differing packages:';
$MSG['FOUND_DIFFER']['de'] = 'abweichende Pakete gefunden:';
$MSG['FOUND_DIFFER']['pl'] = 'znalezionych różnic w pakietach:';

?>

@ECHO OFF

:: This processes all translation files and regenerates the LOCALCFG.LNG and
:: DEFLANG.C files.

utf8tocp 437 en_utf8.txt > en.txt
utf8tocp 850 br_utf8.txt > br.txt
utf8tocp 850 de_utf8.txt > de.txt
utf8tocp 850 fr_utf8.txt > fr.txt
utf8tocp maz pl_utf8.txt > pl.txt
utf8tocp 857 tr_utf8.txt > tr.txt

..\svarlang.lib\tlumacz en br de fr pl tr > tlumacz.log

del ??.txt
move /y out.lng ..\localcfg.lng
move /y deflang.c ..\

@ECHO OFF

utf8tocp 437 en_utf8.txt > en.txt
utf8tocp 850 de_utf8.txt > de.txt
utf8tocp 850 fr_utf8.txt > fr.txt
utf8tocp maz pl_utf8.txt > pl.txt
utf8tocp 866 ru_utf8.txt > ru.txt
utf8tocp 857 tr_utf8.txt > tr.txt

..\svarlang\tlumacz en de fr pl ru tr
move /y out.lng ..\sved.lng
move /y deflang.c ..

del ??.txt

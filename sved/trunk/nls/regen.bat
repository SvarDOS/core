@ECHO OFF

utf8tocp 437 en_utf8.txt > en.txt
utf8tocp 850 fr_utf8.txt > fr.txt
utf8tocp maz pl_utf8.txt > pl.txt
utf8tocp 866 ru_utf8.txt > ru.txt

..\svarlang\tlumacz en fr pl ru
move /y out.lng ..\sved.lng
move /y deflang.c ..

del ??.txt

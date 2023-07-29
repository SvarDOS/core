@ECHO OFF

utf8tocp 437 en_utf8.txt > en.txt
utf8tocp maz pl_utf8.txt > pl.txt

..\svarlang\tlumacz en pl
move /y out.lng ..\edit.lng
move /y deflang.c ..

del ??.txt

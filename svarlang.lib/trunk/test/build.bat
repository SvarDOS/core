@echo off

IF EXIST OUT.LNG DEL OUT.LNG
IF EXIST OUTC.LNG DEL OUTC.LNG

..\tlumacz en pl
rename out.lng outc.lng

..\tlumacz /nocomp en pl

wcl -0 -wx -ox -we -ms test.c deflang.c ..\svarlngs.lib

cd fdisk
..\..\tlumacz en de es fr it /nocomp pl tr
del ..\fdisk.lng
copy out.lng ..\fdisk.lng
del ..\deflfdsk.c
copy deflang.c ..\deflfdsk.c
del out.lng

del ..\fdisk_de.lng
..\..\tlumacz /nodef de
copy out.lng ..\fdisk_de.lng
del out.lng
cd ..

wcl -0 -wx -ox -we -ms tim.c deflfdsk.c ..\svarlngs.lib

cd tree
..\..\tlumacz /nodef en de es fi lv pt ru tr
del ..\tree.lng
copy out.lng ..\tree.lng
del out.lng
cd ..

cd install
..\..\tlumacz /nodef en br de fr it pl ru si sv tr
del ..\install.lng
copy out.lng ..\install.lng
del out.lng
cd ..

cd svarcom
..\..\tlumacz /nodef en br de fr pl tr
del ..\svarcom.lng
copy out.lng ..\svarcom.lng
del out.lng
cd ..

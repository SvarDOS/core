@echo off

IF EXIST OUT.LNG DEL OUT.LNG
IF EXIST OUTC.LNG DEL OUTC.LNG

..\tlumacz en pl
rename out.lng outc.lng

..\tlumacz /nocomp en pl

wcl -0 -wx -ox -we -ms test.c deflang.c ..\svarlngs.lib

cd fdisk
..\..\tlumacz en de es fr it pl tr
del ..\fdisk.lng
copy out.lng ..\fdisk.lng
del out.lng

del ..\fdisk_de.lng
..\..\tlumacz de
copy out.lng ..\fdisk_de.lng
cd ..

cd tree
..\..\tlumacz en de es fi lv pt ru tr
del ..\tree.lng
copy out.lng ..\tree.lng
del out.lng
cd ..

cd install
..\..\tlumacz en br de fr it pl ru si sv tr
del ..\install.lng
copy out.lng ..\install.lng
del out.lng
cd ..

cd svarcom
..\..\tlumacz en br de fr pl tr
del ..\svarcom.lng
copy out.lng ..\svarcom.lng
del out.lng
cd ..

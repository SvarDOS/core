@echo off

IF EXIST OUT.LNG DEL OUT.LNG
IF EXIST OUTC.LNG DEL OUTC.LNG

..\tlumacz en pl
rename out.lng outc.lng

..\tlumacz /nocomp en pl

wcl -0 -wx -ox -we -ms test.c deflang.c ..\svarlngs.lib

cd fdisk
..\..\tlumacz en de es fr it pl tr
copy out.lng ..\fdisk.lng
del out.lng
cd ..

cd tree
..\..\tlumacz en de es fi lv pt ru tr
copy out.lng ..\tree.lng
del out.lng
cd ..

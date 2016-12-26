#!/bin/sh

utf8tocp 858 install.it.utf8 > install.it
if [ $? -ne 0 ] ; then
  echo "ERROR! ABORTED!"; exit
fi

utf8tocp maz install.pl.utf8 > install.pl
if [ $? -ne 0 ] ; then
  echo "ERROR! ABORTED!"; exit
fi

utf8tocp 858 install.fr.utf8 > install.fr
if [ $? -ne 0 ] ; then
  echo "ERROR! ABORTED!"; exit
fi

utf8tocp 852 install.si.utf8 > install.si
if [ $? -ne 0 ] ; then
  echo "ERROR! ABORTED!"; exit
fi

utf8tocp 857 install.tr.utf8 > install.tr
if [ $? -ne 0 ] ; then
  echo "ERROR! ABORTED!"; exit
fi

utf8tocp 866 install.ru.utf8 > install.ru
if [ $? -ne 0 ] ; then
  echo "ERROR! ABORTED!"; exit
fi

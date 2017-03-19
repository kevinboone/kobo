#!/bin/bash
WEB=/home/kevin/docs/kzone5/
SOURCE=$WEB/source
TARGET=$WEB/target
make clean
(cd ..; tar cvfz $TARGET/kobo.tar.gz kobo)
cp README_kobo.html $SOURCE
(cd $WEB; ./make.pl kobo)

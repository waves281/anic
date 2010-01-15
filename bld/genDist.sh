#!/bin/sh

### Redistributable package generator script

if test ! -f $1-$2."`cat var/versionStamp.txt`".tar.gz ; then
	echo Packing redistributable...
	tar cf $1-$2."`cat var/versionStamp.txt`".tar $1 $3.gz $4 $5
	gzip -f $1-$2."`cat var/versionStamp.txt`".tar
	echo Done packing to $1-$2."`cat var/versionStamp.txt`".tar.gz
fi
exit

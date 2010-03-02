#!/bin/sh

### Redistributable package generator script
### arguments: $(TARGET) $(VERSION_STRING) $(MAN_PAGE) $(INSTALL_SCRIPT) $(UNINSTALL_SCRIPT)

if test ! -f $1-$2."`cat var/versionStamp.txt`".tar.gz ; then
	echo Packing redistributable...
	ls $1.exe 1> /dev/null 2> /dev/null
	if test $? = 0 ; then
		tar -cf $1-$2."`cat var/versionStamp.txt`".tar $1.exe $3.gz $4 $5
	else
		tar -cf $1-$2."`cat var/versionStamp.txt`".tar $1 $3.gz $4 $5
	fi
	gzip -f $1-$2."`cat var/versionStamp.txt`".tar
	echo Done packing to $1-$2."`cat var/versionStamp.txt`".tar.gz
fi
exit

#!/bin/sh

### Checksum program location script

# Cygwin/Linux
if test -f /usr/bin/sha256sum
then
	echo /usr/bin/sha256sum
	exit
fi

# Mac OSX
if test -f /usr/bin/shasum
then
	echo /usr/bin/shasum
	exit
fi

# FreeBSD
if test -f /sbin/sha256
then
	echo /sbin/sha256
	exit
fi

echo Cannot find checksum program!
exit 1

#!/bin/sh

### Checksum program location script

# Cygwin/Linux
type sha256sum > /dev/null
if test $? = 0
then
	printf "sha256sum"
	exit
fi

# Mac OSX / Sun OS
type shasum > /dev/null
if test $? = 0
then
	printf "shasum"
	exit
fi

# FreeBSD
type sha256 > /dev/null
if test $? = 0
then
	printf "sha256"
	exit
fi

echo Cannot find checksum program! > 2
exit 1

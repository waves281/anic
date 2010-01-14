#!/bin/sh

### Checksum program location script

if test -f /usr/bin/sha256sumc
then
	echo /usr/bin/sha256sum
else
	echo /usr/bin/shasum
fi
exit

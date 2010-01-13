#!/bin/sh

### Checksum program location script

if [ -f /usr/bin/sha256sum ] ; then
	echo /usr/bin/sha256sum
else
	echo /usr/bin/shasum
fi
exit

#!/bin/sh

### Installation authentication script

echo Running installer...

if test -f var/testCertificate.dat
then
	./$*;
else
	echo
	echo Warning: Default test cases have not been passed. To run them, use \'make test\'.
	read -p "Proceed anyway (y/n)? " yn
	case $yn in
		[Yy]* ) ./$*;;
		[Nn]* ) exit;;
		* ) echo "Assuming 'no'.";;
	esac
fi
exit

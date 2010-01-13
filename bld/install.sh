#!/bin/sh

### Installer script

echo Running installer...

if [ -a var/testCertificate.dat ]; then
	echo Installing binary...; cp -f $1 $2/$1; echo Installing manpage...; cp -f $3 /usr/share/man/man1; echo Installation complete.;
else
	echo
	echo Warning: Default test cases have not been passed. To run them, use \'make test\'.
	read -p "Proceed anyway (y/n)? " yn
	case $yn in
		[Yy]* ) echo Installing binary...; cp -f $1 $2/$1; echo Installing manpage...; cp -f $3 /usr/share/man/man1; echo Installation complete.;;
		[Nn]* ) exit;;
		* ) echo "Assuming 'no'.";;
	esac
fi
exit

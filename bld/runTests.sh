#!/bin/sh

### Test case script

echo
echo ...Running default test cases...
echo --------------------------------
./$*
if test $? = 0
then
	echo --------------------------------
	echo Default test cases passed.
	echo
	rm -f ./a.out
	mkdir -p var
	echo OK > var/testCertificate.dat
else
	echo --------------------------------
	echo Failed default test cases!
	echo "NOTE: This is the expected behaviour for now; there's still work to be done."
	echo
	rm -f var/testCertificate.dat
fi
exit 0

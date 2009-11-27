#include "customOperators.h"

#include "mainDefs.h"

// string vector streaming operator
ostream &operator<< (ostream &os, vector<char> &s) {
	for(unsigned int i=0; i < s.size(); i++) {
		os << s[i];
	}
	return os;
}

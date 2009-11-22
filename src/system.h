#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "mainDefs.h"

int printHeader(void) {
	cerr << HEADER_LITERAL;
	return 0;
}

int printUsage(void) {
	cerr << USAGE_LITERAL;
	return 0;
}

int printHelp(void) {
	printHeader();
	printUsage();
	return 0;
}

#define print(s) cout << "ani: " << s << ".\n";
#define printError(s) cerr << "ani: ERROR: " << s << ".\n"
#define printWarning(s) cerr << "ani: warning: " << s << ".\n"

void die(int doErrorPrint) {
	if (doErrorPrint) {
		printError("fatal error -- stop");
	}
	exit(1);
}

void die(void) {
	die(0);
}

#endif

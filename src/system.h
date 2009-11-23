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

#define print(s) cout << PROGRAM_STRING << ": " << s << ".\n";
#define printError(s) cerr << PROGRAM_STRING << ": ERROR: " << s << ".\n"
#define printWarning(s) cerr << PROGRAM_STRING << ": warning: " << s << ".\n"

void die(int errorCode) {
	if (errorCode) {
		printError("fatal error code " << errorCode << " -- stop");
	}
	exit(errorCode);
}

void die(void) {
	die(0);
}

#endif

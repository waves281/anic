#include "system.h"

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

void die(int errorCode) {
	if (errorCode) {
		printError("fatal error code " << errorCode << " -- stop");
	}
	exit(errorCode);
}

void die(void) {
	die(0);
}

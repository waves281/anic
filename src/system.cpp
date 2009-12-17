#include "system.h"

void printHeader(void) {
	cerr << HEADER_LITERAL;
}

void printUsage(void) {
	cerr << USAGE_LITERAL;
}

void printLink(void) {
	cerr << LINK_LITERAL;
}

void printHelp(void) {
	printHeader();
	printUsage();
	printLink();
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

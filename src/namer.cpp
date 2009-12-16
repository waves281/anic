#include "mainDefs.h"
#include "system.h"

#include "namer.h"

SymbolTable *genStdDefs() {

	return NULL;
}

SymbolTable *name(Tree *rootParseme, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {
	// local error code
	int namerErrorCode = 0;

	// initialize the sysmbol table with the the default primitive definitions
	SymbolTable *st = genStdDefs();

	// finally, return to the caller
	if (namerErrorCode) {
		// deallocate the output vector, since we're just going to return null
		delete st;
		return NULL;
	} else {
		return st;
	}
}

#include "mainDefs.h"
#include "system.h"

#include "namer.h"

// symbol table functions

// allocators/deallocators
SymbolTable::SymbolTable(string id, Tree *def, SymbolTable *parent) : id(id), def(def) {
	if (parent != NULL) {
		parent->children.push_back(this);
	}
}

// concatenators
SymbolTable &SymbolTable::operator*=(SymbolTable *st) {
	children.push_back(st);
	st->parent = this;
	return *st;
}

// generate an identifier structure (for the symbol table) from a string
vector<string> genId(char *s) {
	vector<string> retVal;

	char *temp = MALLOC_STRING;
	unsigned int tempIndex = 0;
	for (unsigned int i=0; s[i] != '\0'; i++) {
		if (s[i] != '.') { // if it's not a separator
			// log the current character
			temp[tempIndex] = s[i];
			// advance in the temporary string
			tempIndex++;
		} else { // else if it *is* a separator
			temp[tempIndex] = '\0';
			retVal.push_back(temp);
			tempIndex = 0;
		}
	}
	if (tempIndex != 0) { // log the unterminated remains, if there are any
		temp[tempIndex] = '\0';
		retVal.push_back(temp);
	}
	free(temp);
	return retVal;
}

// main naming functions

SymbolTable *genStdDefs() {
	// standard root
	SymbolTable *retVal = new SymbolTable(STANDARD_LIBRARY_PREFIX, NULL, NULL);
	// standard types
	*retVal *= new SymbolTable("byte", NULL, NULL);
	*retVal *= new SymbolTable("int", NULL, NULL);
	*retVal *= new SymbolTable("float", NULL, NULL);
	*retVal *= new SymbolTable("bool", NULL, NULL);
	*retVal *= new SymbolTable("char", NULL, NULL);
	*retVal *= new SymbolTable("string", NULL, NULL);
	// standard streams
	*retVal *= new SymbolTable("stdin", NULL, NULL);
	*retVal *= new SymbolTable("stdout", NULL, NULL);
	*retVal *= new SymbolTable("stderr", NULL, NULL);
	// return the compiled standard list to the caller
	return retVal;
}

SymbolTable *name(Tree *rootParseme, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {
	// local error code
	int namerErrorCode = 0;

	// initialize the sysmbol table with the the default standard definitions
	SymbolTable *stRoot = genStdDefs();

	// finally, return to the caller
	if (namerErrorCode) {
		// deallocate the output vector, since we're just going to return null
		delete stRoot;
		return NULL;
	} else {
		return stRoot;
	}
}

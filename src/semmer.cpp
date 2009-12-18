#include "mainDefs.h"
#include "system.h"

#include "semmer.h"

// symbol table functions

// allocators/deallocators
SymbolTable::SymbolTable(string id, Tree *def) : id(id), def(def) {}

// concatenators
SymbolTable &SymbolTable::operator*=(SymbolTable *st) {
	children.push_back(st);
	if (st != NULL) {
		st->parent = &children;
		return *st;
	} else {
		return *this;
	}
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

// main semantic analysis functions

SymbolTable *genStdDefs() {
	// standard root
	SymbolTable *retVal = new SymbolTable(STANDARD_LIBRARY_PREFIX, NULL);
	// standard types
	*retVal *= new SymbolTable("node", NULL);
	*retVal *= new SymbolTable("byte", NULL);
	*retVal *= new SymbolTable("int", NULL);
	*retVal *= new SymbolTable("float", NULL);
	*retVal *= new SymbolTable("bool", NULL);
	*retVal *= new SymbolTable("char", NULL);
	*retVal *= new SymbolTable("string", NULL);
	// standard streams
	*retVal *= new SymbolTable("stdin", NULL);
	*retVal *= new SymbolTable("stdout", NULL);
	*retVal *= new SymbolTable("stderr", NULL);
	// standard library
	// standard containers
	*retVal *= new SymbolTable("stack", NULL);
	*retVal *= new SymbolTable("map", NULL);
	// standard filters
	*retVal *= new SymbolTable("filter", NULL);
	*retVal *= new SymbolTable("sort", NULL);
	// standard generators
	*retVal *= new SymbolTable("gen", NULL);
	// return the compiled standard list to the caller
	return retVal;
}

int sem(Tree *rootParseme, deque<SymbolTable *> &stRoot, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {
	// local error code
	int semmerErrorCode = 0;

	// perform main identifier analysis on the supplied root parse tree



	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
